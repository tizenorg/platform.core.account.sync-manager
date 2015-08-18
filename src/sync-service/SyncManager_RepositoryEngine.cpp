/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/**
 * @file	SyncManager_RepositoryEngine.cpp
 * @brief	This is the implementation file for the RepositoryEngine class.
 */

#include <sys/time.h>
#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <errno.h>
#include <fstream>
#include <sstream>
#include <tzplatform_config.h>
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncJobQueue.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncAdapterAggregator.h"
#include "sync-log.h"
#include "sync-error.h"


/*namespace _SyncManager
{
*/

#define SYNC_DIRECTORY "sync-manager";
#define SYNC_DATA_DIR tzplatform_mkpath(TZ_USER_DATA, "/sync-manager")
#define PATH_ACCOUNT tzplatform_mkpath(TZ_USER_DATA,"/sync-manager/accounts.xml")
#define PATH_SYNCJOBS tzplatform_mkpath(TZ_USER_DATA,"/sync-manager/syncjobs.xml")
#define PATH_SYNCADAPTERS tzplatform_mkpath(TZ_USER_DATA,"/sync-manager/syncadapters.xml")
#define PATH_STATUS tzplatform_mkpath(TZ_USER_DATA,"/sync-manager/statusinfo.bin")

#ifndef MAX
#define MAX(a, b) a>b?a:b
#endif


//xml Tags Definitions
//For Accounts.xml
//static char PATH_ACCOUNT[] = "/opt/usr/data/sync-manager/accounts.xml";
//static char PATH_SYNCJOBS[] = "/opt/usr/data/sync-manager/syncjobs.xml";
//static char PATH_SYNCADAPTERS[] = "/opt/usr/data/sync-manager/syncadapters.xml";
//static char PATH_STATUS[] = "/opt/usr/data/sync-manager/statusinfo.bin";
static const xmlChar _VERSION[]				= "1.0";


static const xmlChar XML_NODE_ACCOUNTS[]						= "accounts";
static const xmlChar XML_ATTR_NEXT_CAPABILITY_ID[]				= "nextCapabilityId";
static const xmlChar XML_ATTR_SYNC_RANDOM_OFFSET[]				= "randomOffsetInSec";

static const xmlChar XML_NODE_CAPABILITY[]						= "capability";
static const xmlChar XML_ATTR_CAPABILITY_ID[]					= "id";
static const xmlChar XML_ATTR_APP_ID[]							= "appId";
static const xmlChar XML_ATTR_ENABLED[]							= "enabled";
static const xmlChar XML_ATTR_ACCOUNT_ID[]						= "accountId";
static const xmlChar XML_ATTR_ACCOUNT_TYPE[]					= "accountType";
static const xmlChar XML_ATTR_CAPABILITY[]						= "capability";
static const xmlChar XML_ATTR_SYNCABLE[]						= "syncable";

static const xmlChar XML_NODE_PERIODIC_SYNC[]					= "periodicSync";
static const xmlChar XML_ATTR_PERIODIC_SYNC_PEIOD[]				= "period";
static const xmlChar XML_ATTR_PERIODIC_SYNC_FLEX[]				= "flex";

static const xmlChar XML_NODE_SYNC_EXTRA[]						= "extra";
static const xmlChar XML_ATTR_SYNC_EXTRA_KEY[]					= "key";
static const xmlChar XML_ATTR_SYNC_EXTRA_VALUE[]				= "value";

static const xmlChar XML_NODE_JOBS[]							= "jobs";
static const xmlChar XML_ATTR_JOBS_COUNT[]						= "count";

static const xmlChar XML_NODE_SYNC_JOB[]						= "job";
static const xmlChar XML_ATTR_JOB_APP_ID[]						= "appId";
static const xmlChar XML_ATTR_JOB_ACCOUNT_ID[]					= "accountId";
static const xmlChar XML_ATTR_JOB_REASON[]						= "reason";
static const xmlChar XML_ATTR_JOB_SOURCE[]						= "source";
static const xmlChar XML_ATTR_JOB_CAPABILITY[]					= "capability";
static const xmlChar XML_ATTR_JOB_KEY[]							= "key";
static const xmlChar XML_ATTR_JOB_PARALLEL_SYNC_ALLOWED[]		= "parallelSyncAllowed";
static const xmlChar XML_ATTR_JOB_BACKOFF[]						= "backoff";
static const xmlChar XML_ATTR_JOB_DELAY_UNTIL[]					= "delayUntil";
static const xmlChar XML_ATTR_JOB_FLEX_TIME[]					= "flex";

static const xmlChar XML_NODE_SYNCADAPTERS[]					= "sync-adapters";
static const xmlChar XML_ATTR_SYNCADAPTERS_COUNT[]				= "count";

static const xmlChar XML_NODE_SYNCADAPTER[]						= "sync-adapter";
static const xmlChar XML_ATTR_SYNCADAPTER_SERVICE_APP_ID[]		= "service-app-id";
static const xmlChar XML_ATTR_SYNCADAPTER_ACCOUNT_PROVIDER_ID[]	= "account-provider-id";
static const xmlChar XML_ATTR_SYNCADAPTER_CAPABILITY[]			= "capability";

static const string SYNCABLE_STATE_UNKNOWN						= "unknown";

const long RepositoryEngine::NOT_IN_BACKOFF_MODE = -1;
const long RepositoryEngine::DEFAULT_PERIOD_SEC = 24*60*60;	// 1 day
const double RepositoryEngine::DEFAULT_FLEX_PERCENT = 0.04;		// 4 percent
const long RepositoryEngine::DEFAULT_MIN_FLEX_ALLOWED_SEC = 5;	// 5 seconds

#define FOR_ACCOUNT_LESS_SYNC -2

RepositoryEngine* RepositoryEngine::__pInstance = NULL;


RepositoryEngine*
RepositoryEngine::GetInstance(void)
{
	if (!__pInstance)
	{
		__pInstance = new (std::nothrow) RepositoryEngine();
		if (__pInstance == NULL)
		{
			LOG_LOGD("Failed to construct RepositoryEngine");
			return NULL;
		}
	}

	return __pInstance;
}


RepositoryEngine::~RepositoryEngine(void)
{
	pthread_mutex_destroy(&__capabilityInfoMutex);
}


RepositoryEngine::RepositoryEngine(void)
	: __nextCapabilityId(0)
	, __randomOffsetInSec(0)
{
	ReadSyncAdapters();
	ReadAccountData();
	ReadStatusData();
	ReadSyncJobsData();

	if (pthread_mutex_init(&__capabilityInfoMutex, NULL) != 0)
	{
		LOG_LOGD("\n __syncJobQueueMutex init failed\n");
		return;
	}
}

//Test method
/*
static void
bndl_iterator_test(const char* pKey, const char* pVal, void* pData)
{
	LOG_LOGD("SyncJobQueue sync extra key %s val %s", pKey, pVal);
}
*/

void
RepositoryEngine::ReadAccountData(void)
{
	LOG_LOGD("Read accounts data");

	int maxCapabilityId = -1;
	//Parse the Xml file
	const char* pDocName;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	pDocName = PATH_ACCOUNT;
	doc = xmlParseFile(pDocName);
	if (doc == NULL)
	{
		LOG_LOGD("Failed to parse Account.xml.");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL)
	{
		LOG_LOGD("Found empty document while parsing Account.xml.");
		xmlFreeDoc(doc);
		return;
	}

	//Parse the Accounts
	LOG_LOGD("RepositoryEngine::Root name = %s", cur->name);
	if (xmlStrcmp(cur->name, XML_NODE_ACCOUNTS))
	{
		LOG_LOGD("Found empty document while parsing Account.xml.");
		xmlFreeDoc(doc);
		return;
	}
	else
	{
		xmlChar* pNextCapabilityId;
		xmlChar* pRandomOffsetInSec;

		pNextCapabilityId = xmlGetProp(cur, XML_ATTR_NEXT_CAPABILITY_ID);
		int id = (pNextCapabilityId == NULL) ? 0 : atoi((char*)pNextCapabilityId);
		__nextCapabilityId = MAX(__nextCapabilityId, id);

		pRandomOffsetInSec = xmlGetProp(cur, XML_ATTR_SYNC_RANDOM_OFFSET);
		__randomOffsetInSec = (pRandomOffsetInSec == NULL) ? 0 : atol((char*)pRandomOffsetInSec);

		if (__randomOffsetInSec == 0)
		{
			struct timeval timeVal;
			gettimeofday(&timeVal, NULL);
			long long currentTimeInMilliSecs = timeVal.tv_sec * 1000LL + timeVal.tv_usec / 1000;

			srand(currentTimeInMilliSecs);
			__randomOffsetInSec = rand() % 86400;
		}

		cur = cur->xmlChildrenNode;
		LOG_LOGD("RepositoryEngine::child name = %s", (char*)(cur->name));
		while (cur != NULL)
		{
			if (!xmlStrcmp(cur->name, XML_NODE_CAPABILITY))
			{
				LOG_LOGD("going to call ParseCapabilities for child name %s", (char*)(cur->name));
				CapabilityInfo* pCapabilityInfo = ParseCapabilities(cur);
				if (pCapabilityInfo == NULL)
				{
					LOG_LOGD("ParseCapabilities returned NULL");
					cur = cur->next;
					continue;
				}

				maxCapabilityId = (pCapabilityInfo->id > maxCapabilityId) ? pCapabilityInfo->id : maxCapabilityId;

			}
			cur = cur->next;
		}
	}

	__nextCapabilityId = MAX(maxCapabilityId + 1, __nextCapabilityId);
	xmlFreeDoc(doc);

	// Test code
	/*LOG_LOGD("__capabilities size %d", __capabilities.size());
	map<int, CapabilityInfo*>::iterator it;
	for (it=__capabilities.begin(); it!=__capabilities.end(); it++)
	{
		CapabilityInfo* pCapabilityInfo = it->second;
		LOG_LOGD("capability info %d, id %d", i+1, pCapabilityInfo->id);
		LOG_LOGD("periodic sync list size %d", pCapabilityInfo->periodicSyncList.size());
		for (int j=0; j<pCapabilityInfo->periodicSyncList.size(); j++)
		{
			PeriodicSyncJob* pJob = pCapabilityInfo->periodicSyncList[j];
			string str;
			str.append("[");
			bundle_iterate(pJob->pExtras, bndl_iterator_test, &str);
			str.append("]");
		}
		int accId;
		if (account_get_account_id(pCapabilityInfo->accountHandle, &accId) == 0)
		{
			LOG_LOGD("capability info %d, accId %d", i+1, accId);
		}
		LOG_LOGD("capability info %d, appId %s", i+1, pCapabilityInfo->appId.c_str());
		LOG_LOGD("capability info %d, capability %s", i+1, pCapabilityInfo->capability.c_str());
		LOG_LOGD("capability info %d, syncable %d", i+1, pCapabilityInfo->syncable);
	}*/
	//Till here
}


void
RepositoryEngine::ReadStatusData(void)
{
	LOG_LOGD("Read Status Data");

	ifstream inFile(PATH_STATUS, ios::in|ios::binary);
	if (!inFile.is_open())
	{
		LOG_LOGD("Failed to open bin file");
		return;
	}

	string buff;

	while (getline(inFile, buff, '#'))
	{
		LOG_LOGD("Status reading buff %s", buff.c_str());
		SyncStatusInfo* pSyncStatusInfo = new (std::nothrow) SyncStatusInfo(buff);
		LOG_LOGE_VOID(pSyncStatusInfo, "Failed to construct SyncStatusInfo");

		int capabilityId = pSyncStatusInfo->capabilityId;

		map<int, CapabilityInfo*>::iterator it = __capabilities.find(capabilityId);
		if (it == __capabilities.end())
		{
			LOG_LOGD("Unknown status found");
			delete pSyncStatusInfo;
			pSyncStatusInfo = NULL;
			break;
		}

		__syncStatus.insert(make_pair(capabilityId, pSyncStatusInfo));
	}

	inFile.close();
}


void
RepositoryEngine::ReadSyncJobsData(void)
{
	LOG_LOGD("Read Sync jobs");

	//Parse the Xml file
	const char* pDocName;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	pDocName = PATH_SYNCJOBS;
	doc = xmlParseFile(pDocName);

	if (doc == NULL)
	{
		LOG_LOGD("Failed to parse syncjobs.xml.");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL)
	{
		LOG_LOGD("Found empty document while parsing syncjobs.xml.");
		xmlFreeDoc(doc);
		return;
	}

	//Parse sync jobs
	if (xmlStrcmp(cur->name, XML_NODE_JOBS))
	{
		LOG_LOGD("Found empty document while parsing syncjobs.xml.");
		xmlFreeDoc(doc);
		return;
	}
	else
	{
		xmlChar* pJobsCount;

		pJobsCount = xmlGetProp(cur, XML_ATTR_JOBS_COUNT);
		int count = (pJobsCount == NULL) ? 0 : atoi((char*)pJobsCount);

		LOG_LOGD("ReadSyncJobsData, pJobsCount %d", count);

		cur = cur->xmlChildrenNode;
		while (cur != NULL)
		{
			if (!xmlStrcmp(cur->name, XML_NODE_SYNC_JOB))
			{
				LOG_LOGD("going to call ParseJobs for child name %s", (char*)(cur->name));

				SyncJob* pSyncJob = ParseSyncJobsN(cur);
				if (!pSyncJob)
				{
					cur = cur->next;
					LOG_LOGD("Failed to parse sync job");
					continue;
				}

				int ret = SyncManager::GetInstance()->AddToSyncQueue(pSyncJob);
				if (ret != SYNC_ERROR_NONE)
				{
					delete pSyncJob;
					pSyncJob = NULL;
					cur = cur->next;
					LOG_LOGD("Failed to add sync job to queue");
					continue;
				}
				delete pSyncJob;
				pSyncJob = NULL;
			}
			cur = cur->next;
		}
	}

	xmlFreeDoc(doc);

	// Test code
	/*
	int size = SyncManager::GetInstance()->GetSyncJobQueue()->GetSyncJobQueue().size();
	map<const string, SyncJob*> queue = SyncManager::GetInstance()->GetSyncJobQueue()->GetSyncJobQueue();
	map<const string, SyncJob*>::iterator it;
	LOG_LOGD("SyncJobQueue size %d", size);
	for (it = queue.begin(); it != queue.end(); it++)
	{
		SyncJob* pJob = it->second;
		LOG_LOGD("SyncJobQueue appId %s", pJob->appId.c_str());
		LOG_LOGD("SyncJobQueue capability %s", pJob->capability.c_str());
		LOG_LOGD("SyncJobQueue backoff %ld, delay %ld", pJob->backoff, pJob->delayUntil);
		LOG_LOGD("SyncJobQueue key %s, reason %d, source %d", pJob->key.c_str(), pJob->reason, pJob->syncSource);
		//string str;
		//str.append("[");
		//bundle_iterate(pJob->pExtras, bndl_iterator_test, &str);
		//str.append("]");
	}
	*/
	//Till here
}

void
RepositoryEngine::ReadSyncAdapters(void)
{
	LOG_LOGD("Reading sync adapters");

	//Parse the Xml file
	const char* pDocName;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	pDocName = PATH_SYNCADAPTERS;
	doc = xmlParseFile(pDocName);

	if (doc == NULL)
	{
		LOG_LOGD("Failed to parse syncadapters.xml.");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL)
	{
		LOG_LOGD("Found empty document while parsing syncadapters.xml.");
		xmlFreeDoc(doc);
		return;
	}

	//Parse sync jobs
	if (xmlStrcmp(cur->name, XML_NODE_SYNCADAPTERS))
	{
		LOG_LOGD("Found empty document while parsing syncadapters.xml.");
		xmlFreeDoc(doc);
		return;
	}
	else
	{
		xmlChar* pSACount;

		pSACount = xmlGetProp(cur, XML_ATTR_SYNCADAPTERS_COUNT);
		int count = (pSACount == NULL) ? 0 : atoi((char*)pSACount);

		LOG_LOGD("sync adapter count %d", count);

		SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();

		cur = cur->xmlChildrenNode;
		while (cur != NULL)
		{
			if (!xmlStrcmp(cur->name, XML_NODE_SYNCADAPTER))
			{
				xmlChar* pServiceAppId = xmlGetProp(cur, XML_ATTR_SYNCADAPTER_SERVICE_APP_ID);
				xmlChar* pAccountProviderId = xmlGetProp(cur, XML_ATTR_SYNCADAPTER_ACCOUNT_PROVIDER_ID);
				xmlChar* pCapability = xmlGetProp(cur, XML_ATTR_SYNCADAPTER_CAPABILITY);

				pAggregator->AddSyncAdapter((char*)pAccountProviderId, (char*)pServiceAppId, (char*)pCapability);
			}
			cur = cur->next;
		}
		pAggregator->dumpSyncAdapters();
	}

	xmlFreeDoc(doc);

	LOG_LOGD("sync adapters are initialized");
}


static void
bndl_iterator(const char* pKey, const char* pVal, void* pData)
{
	xmlNodePtr parentNode = *((xmlNodePtr*)pData);

	xmlNodePtr extraNode = xmlNewChild(parentNode, NULL, XML_NODE_SYNC_EXTRA, NULL);
	xmlNewProp(extraNode, XML_ATTR_SYNC_EXTRA_KEY, (const xmlChar*)pKey);
	xmlNewProp(extraNode, XML_ATTR_SYNC_EXTRA_VALUE, (const xmlChar*)pVal);
}


void
RepositoryEngine::WriteAccountData(void)
{
	LOG_LOGD("Starts");

	xmlDocPtr doc;
	xmlNodePtr rootNode, authorityNode, periodicSyncNode;
	stringstream ss;

	doc = xmlNewDoc(_VERSION);

	rootNode = xmlNewNode(NULL, XML_NODE_ACCOUNTS);
	xmlDocSetRootElement(doc, rootNode);
	ss<<__nextCapabilityId;
	xmlNewProp(rootNode, XML_ATTR_NEXT_CAPABILITY_ID, (const xmlChar*)ss.str().c_str());
	ss.str(string());
	ss<<__randomOffsetInSec;
	xmlNewProp(rootNode, XML_ATTR_SYNC_RANDOM_OFFSET, (const xmlChar*)ss.str().c_str());
	ss.str(string());

	map<int, CapabilityInfo*>::iterator it;
	for (it = __capabilities.begin(); it != __capabilities.end(); it++)
	{
		CapabilityInfo* pCapabilityInfo = it->second;
		if (!pCapabilityInfo)
		{
			continue;
		}
		authorityNode = xmlNewChild(rootNode, NULL, XML_NODE_CAPABILITY, NULL);
		ss<<pCapabilityInfo->id;
		xmlNewProp(authorityNode, XML_ATTR_CAPABILITY_ID, (const xmlChar*)ss.str().c_str());
		ss.str(string());
		xmlNewProp(authorityNode, XML_ATTR_APP_ID, (const xmlChar*)(pCapabilityInfo->appId.c_str()));
		xmlNewProp(authorityNode, XML_ATTR_ENABLED, (pCapabilityInfo->isEnabled) ? (const xmlChar*)"true" : (const xmlChar*)"false");

		if (pCapabilityInfo->accountHandle)
		{
			int accountId;
			account_h account = pCapabilityInfo->accountHandle;
			int ret = account_get_account_id(account, &accountId);
			if (ret == 0)
			{
				ss<<accountId;
			}
			else
			{
				ss<<-1;
			}
		}
		else
		{
			ss<<FOR_ACCOUNT_LESS_SYNC;
		}
		xmlNewProp(authorityNode, XML_ATTR_ACCOUNT_ID, (const xmlChar*)ss.str().c_str());
		ss.str(string());
		//TODO account type
		xmlNewProp(authorityNode, XML_ATTR_CAPABILITY, (const xmlChar*)(pCapabilityInfo->capability.c_str()));
		if (pCapabilityInfo->syncable < 0)
		{
			xmlNewProp(authorityNode, XML_ATTR_SYNCABLE, (const xmlChar*)(SYNCABLE_STATE_UNKNOWN.c_str()));
		}
		else
		{
			xmlNewProp(authorityNode, XML_ATTR_SYNCABLE,
					(pCapabilityInfo->syncable != 0) ? (const xmlChar*)"true" : (const xmlChar*)"false");
		}

		//Write periodic syncs for this capability info now
		for (unsigned int j = 0; j < pCapabilityInfo->periodicSyncList.size(); j++)
		{
			PeriodicSyncJob* pPeriodicSync = pCapabilityInfo->periodicSyncList[j];
			if (!pPeriodicSync)
			{
				continue;
			}

			periodicSyncNode = xmlNewChild(authorityNode, NULL, XML_NODE_PERIODIC_SYNC, NULL);
			ss<<pPeriodicSync->period;
			xmlNewProp(periodicSyncNode, XML_ATTR_PERIODIC_SYNC_PEIOD, (const xmlChar*)ss.str().c_str());
			ss.str(string());
			ss<<pPeriodicSync->flexTime;
			xmlNewProp(periodicSyncNode, XML_ATTR_PERIODIC_SYNC_FLEX, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			if (pPeriodicSync->pExtras)
			{
				bundle_iterate(pPeriodicSync->pExtras, bndl_iterator, &periodicSyncNode);
			}
		}
	}
	int ret = xmlSaveFormatFileEnc(PATH_ACCOUNT, doc, "UTF-8" , 1 );
	if (ret < 0)
	{
		LOG_LOGD("Failed to write account data, error %d, errorno %d", ret, errno);
	}
	xmlFreeDoc(doc);
	LOG_LOGD("Ends");
}


void
RepositoryEngine::WriteStatusData(void)
{
	LOG_LOGD("Starts");

	ofstream outFile(PATH_STATUS, ios::out|ios::binary);//|ios::app
	if (!outFile.is_open())
	{
		LOG_LOGD("failed to open bin file");
		return;
	}
	string buff;

	map<int, SyncStatusInfo*>::iterator it;
	for (it = __syncStatus.begin(); it != __syncStatus.end(); it++)
	{
		SyncStatusInfo* pSyncStatusInfo = it->second;
		buff = pSyncStatusInfo->GetStatusInfoString();
		if (!buff.empty())
		{
			buff.append("#");
			LOG_LOGD("status writing %s", buff.c_str());
			outFile.write(buff.c_str(),buff.size());
		}
	}

	//Below code is for testing for multi sync status
	/*for (it = __syncStatus.begin(); it != __syncStatus.end(); it++)
	{
		SyncStatusInfo* pSyncStatusInfo = it->second;

		buff = pSyncStatusInfo->GetStatusInfoString();
		buff.append("#");
		//buff.append("\n");
		LOG_LOGD("StatusData writing %s", buff.c_str());
		outFile.write(buff.c_str(),buff.size());
	}*/

	outFile.close();

	LOG_LOGD("Ends");
}


void
RepositoryEngine::WriteSyncJobsData(void)
{
	LOG_LOGD("Starts");

	xmlDocPtr doc;
	xmlNodePtr rootNode, jobNode;
	stringstream ss;

	doc = xmlNewDoc(_VERSION);

	rootNode = xmlNewNode(NULL, XML_NODE_JOBS);
	xmlDocSetRootElement(doc, rootNode);

	SyncJobQueue* pJobQueue = SyncManager::GetInstance()->GetSyncJobQueue();
	if (!pJobQueue)
	{
		LOG_LOGD("Failed to get job queue, skip writing to file");
		xmlFreeDoc(doc);
		return;
	}

	map<const string, SyncJob*> jobQueue = pJobQueue->GetSyncJobQueue();
	if (jobQueue.size() == 0)
	{
		LOG_LOGD("No job found");
		xmlFreeDoc(doc);
		return;
	}

	ss<<jobQueue.size();
	xmlNewProp(rootNode, XML_ATTR_JOBS_COUNT, (const xmlChar*)ss.str().c_str());
	ss.str(string());

	map<const string, SyncJob*>::iterator it;
	for (it = jobQueue.begin(); it != jobQueue.end(); it++)
	{
		SyncJob* pJob = it->second;
		if (!pJob)
		{
			continue;
		}
		jobNode = xmlNewChild(rootNode, NULL, XML_NODE_SYNC_JOB, NULL);
		xmlNewProp(jobNode, XML_ATTR_JOB_APP_ID, (const xmlChar*)(pJob->appId.c_str()));

		account_h account = pJob->account;
		int accountId;
		int ret = account_get_account_id(account, &accountId);
		if (ret == 0)
		{
			ss<<accountId;
		}
		else
		{
			ss<<-1;
		}
		xmlNewProp(jobNode, XML_ATTR_JOB_ACCOUNT_ID, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		ss<<((int)pJob->reason);
		xmlNewProp(jobNode, XML_ATTR_JOB_REASON, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		ss<<((int)pJob->syncSource);
		xmlNewProp(jobNode, XML_ATTR_JOB_SOURCE, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		xmlNewProp(jobNode, XML_ATTR_JOB_CAPABILITY, (const xmlChar*)pJob->capability.c_str());
		xmlNewProp(jobNode, XML_ATTR_JOB_KEY, (const xmlChar*)pJob->key.c_str());
		xmlNewProp(jobNode, XML_ATTR_JOB_PARALLEL_SYNC_ALLOWED,
				   pJob->isParallelSyncAllowed ? (const xmlChar*)"true" : (const xmlChar*)"false");

		ss<<pJob->backoff;
		xmlNewProp(jobNode, XML_ATTR_JOB_BACKOFF, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		ss<<pJob->delayUntil;
		xmlNewProp(jobNode, XML_ATTR_JOB_DELAY_UNTIL, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		ss<<pJob->flexTime;
		xmlNewProp(jobNode, XML_ATTR_JOB_FLEX_TIME, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		if (pJob->pExtras)
		{
			bundle_iterate(pJob->pExtras, bndl_iterator, &jobNode);
		}
	}

	int ret = xmlSaveFormatFileEnc(PATH_SYNCJOBS, doc, "UTF-8" , 1 );
	if (ret < 0)
	{
		LOG_LOGD("Failed to write account data, error %d, errno %d", ret, errno);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();

	LOG_LOGD("Ends");
}


void
RepositoryEngine::WriteSyncAdapters(void)
{
	LOG_LOGD(" Starts");

	xmlDocPtr doc;
	xmlNodePtr rootNode, saNode;
	stringstream ss;

	doc = xmlNewDoc(_VERSION);

	rootNode = xmlNewNode(NULL, XML_NODE_SYNCADAPTERS);
	xmlDocSetRootElement(doc, rootNode);

	SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();
	if (!pAggregator)
	{
		LOG_LOGD("Failed to get sync adapter aggregator, skip writing to file");
		xmlFreeDoc(doc);
		return;
	}

	ss << pAggregator->__syncAdapterList.size();
	xmlNewProp(rootNode, XML_ATTR_SYNCADAPTERS_COUNT, (const xmlChar*)ss.str().c_str());
	if (pAggregator->__syncAdapterList.size() != 0)
	{
		ss.str(string());
		for (multimap<string,SyncAdapterAggregator::SyncAdapter*>::iterator it = pAggregator->__syncAdapterList.begin(); it != pAggregator->__syncAdapterList.end(); ++it)
		{
			SyncAdapterAggregator::SyncAdapter* pSyncAdapter = it->second;
			if (!pSyncAdapter)
			{
				continue;
			}
			saNode = xmlNewChild(rootNode, NULL, XML_NODE_SYNCADAPTER, NULL);
			xmlNewProp(saNode, XML_ATTR_SYNCADAPTER_SERVICE_APP_ID, (const xmlChar*)(pSyncAdapter->__pAppId));
			xmlNewProp(saNode, XML_ATTR_SYNCADAPTER_ACCOUNT_PROVIDER_ID, (const xmlChar*)(pSyncAdapter->__pAccountProviderId));
			xmlNewProp(saNode, XML_ATTR_SYNCADAPTER_CAPABILITY, (const xmlChar*)(pSyncAdapter->__pCapability));
		}
	}

	int ret = xmlSaveFormatFileEnc(PATH_SYNCADAPTERS, doc, "UTF-8" , 1 );
	if (ret < 0)
	{
		LOG_LOGD("Failed to write sync adapter data, error %d", ret);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();

	LOG_LOGD(" Ends");
}


CapabilityInfo*
RepositoryEngine::ParseCapabilities(xmlNodePtr cur)
{
	LOG_LOGD("Starts");

	CapabilityInfo* pCapabilityInfo = NULL;

	xmlChar* pId = xmlGetProp(cur, XML_ATTR_CAPABILITY_ID);
	int id = (pId == NULL) ? -1 : atoi((char*)pId);
	if (id >= 0)
	{
		xmlChar* pCapability = xmlGetProp(cur, XML_ATTR_CAPABILITY);
		xmlChar* pEnabled = xmlGetProp(cur, XML_ATTR_ENABLED);
		xmlChar* pSyncable = xmlGetProp(cur, XML_ATTR_SYNCABLE);
		xmlChar* pAccId = xmlGetProp(cur, XML_ATTR_ACCOUNT_ID);
		//xmlChar* pAccType = xmlGetProp(cur, XML_ATTR_ACCOUNT_TYPE);
		xmlChar* pAppId = xmlGetProp(cur, XML_ATTR_APP_ID);

		map<int, CapabilityInfo*>::iterator it = __capabilities.find(id);
		if (it != __capabilities.end())
		{
			pCapabilityInfo = it->second;
		}

		if (!pCapabilityInfo)
		{
			account_h account = NULL;
			if ( pAccId != NULL)
			{
				int accId = atoi((char*)pAccId);
				int pid = SyncManager::GetInstance()->GetAccountPid(accId);

				int ret = account_create(&account);
				KNOX_CONTAINER_ZONE_ENTER(pid);
				ret =  account_query_account_by_account_id(accId, &account);
				KNOX_CONTAINER_ZONE_EXIT();
				if (ret != 0)
				{
					LOG_LOGD("No such ID %d present in account database, error %d", accId, ret);
					return pCapabilityInfo;
				}
			}

			pCapabilityInfo = GetOrCreateCapabilityLocked((char*)pAppId, account, (char*)pCapability, id, false);
		}

		if (pCapabilityInfo)
		{
			pCapabilityInfo->periodicSyncList.clear();
			pCapabilityInfo->isEnabled = (pEnabled == NULL || strcmp((char*)pEnabled, "true") == 0);
			if (pSyncable != NULL && strcmp((char*)pSyncable, SYNCABLE_STATE_UNKNOWN.c_str()) == 0)
			{
				pCapabilityInfo->syncable = -1;
			}
			else
			{
				pCapabilityInfo->syncable = (pSyncable == NULL || strcmp((char*)pSyncable, "true") == 0) ? 1 : 0;
			}

			//Read periodic sync list
			cur = cur->xmlChildrenNode;
			while (cur != NULL)
			{
				if (!xmlStrcmp(cur->name, XML_NODE_PERIODIC_SYNC))
				{
					ParsePeriodicSyncs(cur, pCapabilityInfo);
				}
				cur = cur->next;
			}
		}
	}

	if (!pCapabilityInfo)
	{
		LOG_LOGD("pCapabilityInfo is NULL");
	}

	return pCapabilityInfo;
}


void
RepositoryEngine::ParsePeriodicSyncs(xmlNodePtr cur, CapabilityInfo *pCapabilityInfo)
{
	LOG_LOGD("Parsing Starts");

	PeriodicSyncJob* pPeriodicSync = NULL;

	xmlChar* pPeriod = xmlGetProp(cur, XML_ATTR_PERIODIC_SYNC_PEIOD);
	xmlChar* pFlex = xmlGetProp(cur, XML_ATTR_PERIODIC_SYNC_FLEX);

	if (!pPeriod)
	{
		LOG_LOGD("period can not be null");
		return;
	}

	long period = atol((char*)pPeriod);
	long flex;

	if (!pFlex)
	{
		flex = CalculateDefaultFlexTime(period);
	}
	else
	{
		flex = atol((char*)pFlex);
	}

	bundle* pExtra = bundle_create();

	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		ParseExtras(cur, pExtra);
		cur = cur->next;
	}

	pPeriodicSync = new PeriodicSyncJob(pCapabilityInfo->accountHandle, pCapabilityInfo->capability, pExtra, period, flex);
	if (pPeriodicSync)
	{
		pCapabilityInfo->periodicSyncList.push_back(pPeriodicSync);
	}
	else
	{
		LOG_LOGD("Failed to construct PeriodicSyncJob");
	}

	bundle_free(pExtra);
}


void
RepositoryEngine::ParseExtras(xmlNodePtr cur, bundle* pExtra)
{
	xmlChar* pKey = xmlGetProp(cur, XML_ATTR_SYNC_EXTRA_KEY);
	xmlChar* pVal = xmlGetProp(cur, XML_ATTR_SYNC_EXTRA_VALUE);

	if (!pKey || !pVal)
	{
		return;
	}

	bundle_add(pExtra, (char*)pKey, (char*)pVal);
}


SyncJob*
RepositoryEngine::ParseSyncJobsN(xmlNodePtr cur)
{
	SyncJob* pSyncJob = NULL;

	xmlChar* pKey = xmlGetProp(cur, XML_ATTR_JOB_KEY);
	if (!pKey)
	{
		LOG_LOGD("Null key found for a sync job while parsing xml file");
		return pSyncJob;
	}
	xmlChar* pCapability = xmlGetProp(cur, XML_ATTR_JOB_CAPABILITY);
	if (!pCapability)
	{
		LOG_LOGD("Null capability found for a sync job while parsing xml file");
		return pSyncJob;
	}
	xmlChar* pAppId = xmlGetProp(cur, XML_ATTR_JOB_APP_ID);
	xmlChar* pAccId = xmlGetProp(cur, XML_ATTR_JOB_ACCOUNT_ID);
	xmlChar* pSource = xmlGetProp(cur, XML_ATTR_JOB_SOURCE);
	xmlChar* pReason = xmlGetProp(cur, XML_ATTR_JOB_REASON);
	xmlChar* pParallelSyncAllowed = xmlGetProp(cur, XML_ATTR_JOB_PARALLEL_SYNC_ALLOWED);
	xmlChar* pBackOff = xmlGetProp(cur, XML_ATTR_JOB_BACKOFF);
	xmlChar* pDelayUntil = xmlGetProp(cur, XML_ATTR_JOB_DELAY_UNTIL);
	xmlChar* pFlexTime = xmlGetProp(cur, XML_ATTR_JOB_FLEX_TIME);

	string appId = ((char*)pAppId);
	account_h account = NULL;
	if (pAccId != NULL)
	{
		int accId = atoi((char*)pAccId);
		int pid = SyncManager::GetInstance()->GetAccountPid(accId);

		int ret = account_create(&account);
		KNOX_CONTAINER_ZONE_ENTER(pid);
		ret =  account_query_account_by_account_id(accId, &account);
		KNOX_CONTAINER_ZONE_EXIT();
		if (ret != 0)
		{
			LOG_LOGD("No such ID present in account database");
			return pSyncJob;
		}
	}

	SyncSource source = (pSource == NULL) ? SOURCE_USER : (SyncSource)atoi((char*)pSource);
	SyncReason reason = (pReason == NULL) ? REASON_USER_INITIATED : (SyncReason)atoi((char*)pReason);
	string capability = (char*)pCapability;
	bool isParallelSyncAllowed = (pParallelSyncAllowed == NULL) ? false :
									(strcmp((char*)pParallelSyncAllowed, "true") == 0);
	long backOff = (pBackOff == NULL) ? 0 : atol((char*)pBackOff);
	long delayUntil = (pDelayUntil == NULL) ? 0 : atol((char*)pDelayUntil);
	long flex = (pFlexTime == NULL) ? 0 : atol((char*)pFlexTime);

	bundle* pExtra = bundle_create();

	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		ParseExtras(cur, pExtra);
		cur = cur->next;
	}

	pSyncJob = new (std::nothrow) SyncJob(appId, account, capability, pExtra,
												   reason, source, 0, flex, backOff, delayUntil, isParallelSyncAllowed);
	bundle_free(pExtra);
	if (!pSyncJob)
	{
		LOG_LOGD("Failed to construct sync job instance");
		return NULL;
	}

	return pSyncJob;
}


CapabilityInfo*
RepositoryEngine::GetOrCreateCapabilityLocked(string appId, account_h account, const string capability, int id, bool toWriteToXml)
{
	int accId = 0;
	bool account_less_sync = false;
	if (account == NULL && capability.c_str())
	{
		accId = FOR_ACCOUNT_LESS_SYNC;
		account_less_sync = true;
	}
	else
	{
		int ret = account_get_account_id(account, &accId);
		if (ret != 0)
		{
			return NULL;
		}
	}

	AccountInfo* pAccountInfo;
	map<int, AccountInfo*>::iterator it = __accounts.find(accId);
	if (it == __accounts.end())
	{
		pAccountInfo = new (std::nothrow) AccountInfo(account, appId);
		LOG_LOGE_NULL(pAccountInfo, "Failed to construct AccountInfo");

		__accounts.insert(make_pair(accId, pAccountInfo));
	}
	else
	{
		pAccountInfo = it->second;
	}

	string key = account_less_sync ? appId : capability;
	CapabilityInfo* pCapabilityInfo = NULL;
	map<string, CapabilityInfo*>::iterator itr = pAccountInfo->capabilities.find(key);

	if (itr == pAccountInfo->capabilities.end())
	{
		if (id < 0)
		{
			id = __nextCapabilityId;
			__nextCapabilityId++;
			toWriteToXml = true;
		}

		LOG_LOGD("Creating a new CapabilityInfo with capability : %s", capability.c_str());

		pCapabilityInfo = new (std::nothrow) CapabilityInfo(appId, account, capability, id);
		LOG_LOGE_NULL(pCapabilityInfo, "Failed to construct CapabilityInfo");

		(pAccountInfo->capabilities).insert(make_pair(key, pCapabilityInfo));
		__capabilities.insert(make_pair(id, pCapabilityInfo));

		if (toWriteToXml)
		{
			LOG_LOGD("GetOrCreateCapabilityLocked calling WriteAccountData");
			WriteAccountData();
		}
	}
	else
	{
		pCapabilityInfo = itr->second;
	}

	return pCapabilityInfo;
}


CapabilityInfo*
RepositoryEngine::GetCapabilityLocked(account_h account, const string capability, const char* pTag)
{
	int id;
	int ret = account_get_account_id(account, &id);
	if (ret != 0)
	{
		return NULL;
	}

	map<int, AccountInfo*>::iterator it = __accounts.find(id);
	if (it == __accounts.end())
	{
		if (pTag)
		{
			LOG_LOGD("Account cant be found  : %s", pTag);
		}
		return NULL;
	}

	AccountInfo* pAccountInfo = it->second;

	map<string, CapabilityInfo*>::iterator itr = (pAccountInfo->capabilities).find(capability);
	if (itr == (pAccountInfo->capabilities).end())
	{
		if (pTag)
		{
			LOG_LOGD("Capability cant be found: %s", pTag);
		}
		return NULL;
	}

	return itr->second;
}

void
RepositoryEngine::RemoveAllBackoffValuesLocked(SyncJobQueue* pQueue)
{
	if (pQueue == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return;
	}
	bool changed = false;
	pthread_mutex_lock(&__capabilityInfoMutex);

	map<int, AccountInfo*>::iterator it;
	AccountInfo *accountInfo;
	for (it = __accounts.begin(); it != __accounts.end(); it++)
	{
		accountInfo = it->second;
		map<string, CapabilityInfo*>::iterator it2;
		map<string, CapabilityInfo*> capabilityMap = accountInfo->capabilities;
		CapabilityInfo *capabilityInfo;
		for (it2 = capabilityMap.begin(); it2 != capabilityMap.end(); it2++)
		{
			capabilityInfo = it2->second;
			if (capabilityInfo->backOffTime != BackOffMode::NOT_IN_BACKOFF_MODE || capabilityInfo->backOffDelay != BackOffMode::NOT_IN_BACKOFF_MODE)
			{
				capabilityInfo->backOffTime = BackOffMode::NOT_IN_BACKOFF_MODE;
				capabilityInfo->backOffDelay = BackOffMode::NOT_IN_BACKOFF_MODE;
				pthread_mutex_lock(&SyncManager::GetInstance()->__syncJobQueueMutex);
				pQueue->OnBackoffChanged(accountInfo->account, capabilityInfo->capability, 0);
				pthread_mutex_unlock(&SyncManager::GetInstance()->__syncJobQueueMutex);
				changed = true;
			}
		}
	}
	pthread_mutex_unlock(&__capabilityInfoMutex);

	if (changed)
	{
		//TODO::
		//reportChange(ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS);
	}
}


void
RepositoryEngine::SaveCurrentState(void)
{
	LOG_LOGD("saving states during shutdown");
	pthread_mutex_lock(&__capabilityInfoMutex);
	WriteSyncJobsData();
	WriteStatusData();
	//xmlCleanupParser();
	pthread_mutex_unlock(&__capabilityInfoMutex);
	WriteSyncAdapters();
}


void
RepositoryEngine::CleanUp(string appId)
{
	LOG_LOGD("AppId [%s]", appId.c_str());
	pthread_mutex_lock(&__capabilityInfoMutex);

	vector<int> capabilityIdsToRemove;
	map<int, AccountInfo*>::iterator it;
	for (it = __accounts.begin(); it != __accounts.end();)
	{
		AccountInfo* pAccountInfo = it->second;
		if (pAccountInfo == NULL)
		{
			it++;
			continue;
		}
		if (appId.compare(pAccountInfo->appId) == 0)
		{
			map<string, CapabilityInfo*>::iterator itr;
			for (itr = pAccountInfo->capabilities.begin(); itr != pAccountInfo->capabilities.end(); itr++)
			{
				CapabilityInfo* pCapabilityInfo = itr->second;
				if (pCapabilityInfo)
				{
					capabilityIdsToRemove.push_back(pCapabilityInfo->id);
				}
			}
			__accounts.erase(it++);
			delete pAccountInfo;
			pAccountInfo = NULL;
		}
		else
		{
			it++;
		}
	}
	int size = capabilityIdsToRemove.size();
	if (size > 0)
	{
		while (size > 0)
		{
			size--;
			int id = capabilityIdsToRemove[size];

			// remove from __capabilities
			LOG_LOGD("Going to remove CapabilityInfo[id:%d]", id);
			map<int, CapabilityInfo*>::iterator it = __capabilities.find(id);
			if (it != __capabilities.end())
			{
				CapabilityInfo* pCapabilityInfo = it->second;
				__capabilities.erase(it);
				if (pCapabilityInfo)
				{
					delete pCapabilityInfo;
					pCapabilityInfo = NULL;
				}
			}

			// remove from __syncStatus
			LOG_LOGD("Going to remove SyncStatusInfo[id:%d]", id);
			map<int, SyncStatusInfo*>::iterator itr = __syncStatus.find(id);
			if (itr != __syncStatus.end())
			{
				SyncStatusInfo* pSyncStatusInfo = itr->second;
				__syncStatus.erase(itr);
				if (pSyncStatusInfo)
				{
					delete pSyncStatusInfo;
					pSyncStatusInfo = NULL;
				}
			}
			LOG_LOGD("Done, __capabilities size = %d, __syncStatus size = %d", __capabilities.size(), __syncStatus.size());
		}
		WriteAccountData();
		WriteStatusData();
		WriteSyncJobsData();
	}

	pthread_mutex_unlock(&__capabilityInfoMutex);
	WriteSyncAdapters();
}


backOff*
RepositoryEngine::GetBackoffN(account_h account, const string capability)
{
	if (account == NULL || capability.empty() == true)
	{
		LOG_LOGD("Invalid Parameter");
		return NULL;
	}
	pthread_mutex_lock(&__capabilityInfoMutex);
	CapabilityInfo* pCapabilityInfo = GetCapabilityLocked(account, capability, "GetBackoff");
	if (pCapabilityInfo == NULL || pCapabilityInfo->backOffTime < 0)
	{
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return NULL;
	}
	backOff* pBackOff = new (std::nothrow) backOff;
	if (pBackOff == NULL)
	{
		LOG_LOGD("Failed to construct backoff.");
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return NULL;
	}
	pBackOff->time = pCapabilityInfo->backOffTime;
	pBackOff->delay = pCapabilityInfo->backOffDelay;
	pthread_mutex_unlock(&__capabilityInfoMutex);
	return pBackOff;
}

void
RepositoryEngine::SetBackoffValue(string appId, account_h account, string providerName, long nextSyncTime, long nextDelay)
{

	LOG_LOGD("SetBackoff : appId %s, capability %s, nextSyncTime %ld nextDelay %ld", appId.c_str(), providerName.c_str(), nextSyncTime, nextDelay);
	bool changed = false;
	pthread_mutex_lock(&__capabilityInfoMutex);

	if (account == NULL || providerName.empty())
	{
		map<int, AccountInfo*>::iterator it;
		for (it = __accounts.begin(); it != __accounts.end(); it++)
		{
			AccountInfo *accountInfo = it->second;
			if (account != NULL && !SyncManager::GetInstance()->AreAccountsEqual(account, accountInfo->account)
					&& (appId.compare(accountInfo->appId) != 0))
			{
				continue;
			}
			CapabilityInfo *capabilityInfo;
			map<string, CapabilityInfo*>::iterator it2;
			for (it2 = accountInfo->capabilities.begin(); it2 != accountInfo->capabilities.end(); it2++)
			{
				//AuthorityInfo authorityInfo : accountInfo.authorities.values();
				capabilityInfo = it2->second;
				if (providerName.empty() == false && providerName != capabilityInfo->capability)
				{
					continue;
				}
				if (capabilityInfo->backOffTime != nextSyncTime
						|| capabilityInfo->backOffDelay != nextDelay)
				{
					capabilityInfo->backOffTime = nextSyncTime;
					capabilityInfo->backOffTime = nextDelay;
					changed = true;
				}
			}
		}
	}
	else
	{
		CapabilityInfo* pCapabilityInfo = GetOrCreateCapabilityLocked(appId, account, providerName, -1 /* id */, true);
		if (!pCapabilityInfo)
		{
			pthread_mutex_unlock(&__capabilityInfoMutex);
			return;
		}
		if (pCapabilityInfo->backOffTime == nextSyncTime && pCapabilityInfo->backOffDelay == nextDelay)
		{
			pthread_mutex_unlock(&__capabilityInfoMutex);
			return;
		}
		pCapabilityInfo->backOffTime = nextSyncTime;
		pCapabilityInfo->backOffDelay = nextDelay;
		changed = true;
	}
	pthread_mutex_unlock(&__capabilityInfoMutex);

	if (changed)
	{
	//    reportChange(ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS);
	}
}


long
RepositoryEngine::GetDelayUntilTime(account_h account, const string capability)
{
	/*
	//pthread_mutex_lock(&__capabilityInfoMutex);
	CapabilityInfo* pCapabilityInfo = GetCapabilityLocked(account, capability,
			"GetDelayUntilTime");
	if (pCapabilityInfo == NULL)
	{
		//pthread_mutex_unlock(__pMutex);
		return 0;
	}
	//pthread_mutex_unlock(&__capabilityInfoMutex);
	return pCapabilityInfo->delayUntil;*/
	return 0;
}


PendingJob*
RepositoryEngine::InsertIntoPending(PendingJob* pPendingJob)
{
	//Uncomment below code when implementing pending job logic
	//	//pthread_mutex_lock(&__capabilityInfoMutex);
	//	CapabilityInfo *authority = getOrCreateAuthorityLocked(op->getAccount(), op->getAuthority(), -1, true);
	//	op->__capabilityId = authority->getIdentity();
	//	__pendingJobList.push_back(op);
	//	appendPendingJobLocked(op);
	//	SyncStatusInfo* status = GetOrCreateSyncStatusLocked(authority->getIdentity());
	//	status->pending = true;
	//	//pthread_mutex_unlock(&__capabilityInfoMutex);
	//	//reportChange(SYNC_OBSERVER_TYPE_PENDING);
	return pPendingJob;
}


bool
RepositoryEngine::DeleteFromPending(PendingJob* pPendingJob)
{
	if (pPendingJob == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return false;
	}

	bool res = false;
	//Uncomment below code when implementing pending job logic
   /* pthread_mutex_lock(&__capabilityInfoMutex);
	__pendingJobList.remove(pPendingJob);
	if (__pendingJobList.size() == 0 || __numPendingFinished >= PENDING_FINISH_TO_WRITE)
	{
		//writePendingJobsLocked();
		__numPendingFinished = 0;
	}
	else
	{
		__numPendingFinished++;
	}

	CapabilityInfo* pCapabilityInfo = GetCapabilityLocked(pPendingJob->account, pPendingJob->capability, "deleteFromPending");
	if (!pCapabilityInfo)
	{
		return false;
	}
	list<PendingJob*>::iterator it;
	//TODO Check for usage of N
	//int N = __pendingJobList.size();
	bool morePending = false;
	for (it = __pendingJobList.begin(); it != __pendingJobList.end(); it++)
	{
		PendingJob* cur = *it;

		if (SyncManager::GetInstance()->AreAccountsEqual(cur->account, pPendingJob->account)
				&& strcmp(cur->capability, pPendingJob->capability) == 0)
		{
			morePending = true;
			break;
		}
	}

	if (!morePending)
	{
		SyncStatusInfo* pStatus = GetOrCreateSyncStatusLocked(pCapabilityInfo->id);
		pStatus->isPending = false;
	}

	res = true;

	pthread_mutex_unlock(&__capabilityInfoMutex);*/
	//	reportChange(SYNC_OBSERVER_TYPE_PENDING);
	return res;
}


SyncStatusInfo*
RepositoryEngine::GetOrCreateSyncStatusLocked(int capabilityId)
{
	map<int, SyncStatusInfo*>::iterator it;
	it = __syncStatus.find(capabilityId);
	if (it == __syncStatus.end())
	{
		SyncStatusInfo* pStatus = new (std::nothrow) SyncStatusInfo(capabilityId);
		LOG_LOGE_NULL(pStatus, "Failed to construct SyncStatusInfo");

		__syncStatus.insert(make_pair(capabilityId, pStatus));
		return pStatus;
	}
	return it->second;
}

//Uncomment below code when implementing pending job logic
/*
list<PendingJob*>
RepositoryEngine::GetPendingJobs()
{
	pthread_mutex_lock(&__capabilityInfoMutex);
	list <PendingJob*> jobList = __pendingJobList;
	pthread_mutex_unlock(&__capabilityInfoMutex);
	return jobList;
}
*/

long
RepositoryEngine::CalculateDefaultFlexTime(long period)
{
	long defaultFlexTime;
	if (period < DEFAULT_MIN_FLEX_ALLOWED_SEC)
	{
		// Too small period, can not take risk of adding flex time.
		defaultFlexTime = 0L;
	}
	else if (period < DEFAULT_PERIOD_SEC)
	{
		defaultFlexTime = (long)(period * DEFAULT_FLEX_PERCENT);
	}
	else
	{
		defaultFlexTime = (long)(DEFAULT_PERIOD_SEC * DEFAULT_FLEX_PERCENT);
	}

	return defaultFlexTime;
}


void
RepositoryEngine::AddPeriodicSyncJob(string appId, PeriodicSyncJob* pJob, bool accountLess)
{
	if (appId.empty() == true || pJob == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return;
	}

	if (pJob->period <= 0)
	{
		LOG_LOGD("period should never be <= 0");
	}

	if (pJob->pExtras == NULL)
	{
		LOG_LOGD("extra should never be NULL");
	}


	pthread_mutex_lock(&__capabilityInfoMutex);

	CapabilityInfo* pCapabilityInfo = GetOrCreateCapabilityLocked(appId, pJob->accountHandle, pJob->capability, -1, false);
	if (!pCapabilityInfo)
	{
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}

	bool isAlreadyPresent = false;

	vector< PeriodicSyncJob* >::iterator itr;
	for (itr = pCapabilityInfo->periodicSyncList.begin(); itr != pCapabilityInfo->periodicSyncList.end(); itr++)
	{
		PeriodicSyncJob* pPeriodicSync = *itr;
		if (pPeriodicSync->IsExtraEqual(pJob))
		{
			if (pPeriodicSync->period == pJob->period && pPeriodicSync->flexTime == pJob->flexTime)
			{
				pthread_mutex_unlock(&__capabilityInfoMutex);
				return;
			}
			*itr = pJob;
			isAlreadyPresent = true;
			break;
		}
	}

	if (!isAlreadyPresent)
	{
		pCapabilityInfo->periodicSyncList.push_back(pJob);
		SyncStatusInfo* pSyncStatusInfo = GetOrCreateSyncStatusLocked(pCapabilityInfo->id);

		if (pSyncStatusInfo)
		{
			// While adding an entry to periodic sync array, also add an entry to periodic sync status
			unsigned int index = (pCapabilityInfo->periodicSyncList).size() - 1;
			pSyncStatusInfo->SetPeriodicSyncTime(index, 0LL);
		}
		else
		{
			LOG_LOGD("GetOrCreateSyncStatusLocked returned NULL");
		}

		LOG_LOGD("periodicsync list size %d", pCapabilityInfo->periodicSyncList.size());
		WriteAccountData();
		WriteStatusData();
		pthread_mutex_unlock(&__capabilityInfoMutex);
	}

	SyncManager::GetInstance()->AlertForChange(); //similar to report change
}


void
RepositoryEngine::RemovePeriodicSyncJob(string appId, PeriodicSyncJob* pJob)
{
	if (appId.empty() == true || pJob == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return;
	}
	pthread_mutex_lock(&__capabilityInfoMutex);

	if (pJob->period <= 0)
	{
		LOG_LOGD("period should never be <= 0");
	}

	if (pJob->pExtras == NULL)
	{
		LOG_LOGD("extra should never be NULL");
	}

	CapabilityInfo* pCapabilityInfo = GetOrCreateCapabilityLocked(appId, pJob->accountHandle, pJob->capability, -1, false);
	if (!pCapabilityInfo)
	{
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}

	SyncStatusInfo* pSyncStatusInfo = NULL;
	map<int, SyncStatusInfo*>::iterator it= __syncStatus.find(pCapabilityInfo->id);
	if (it != __syncStatus.end())
	{
		pSyncStatusInfo = it->second;
	}

	bool hasChanged = false;
	vector<PeriodicSyncJob*>::iterator itr;
	unsigned int index = 0;
	for (itr = (pCapabilityInfo->periodicSyncList).begin(); itr != (pCapabilityInfo->periodicSyncList).end();)
	{
		PeriodicSyncJob* pPeriodicSync = *itr;
		if (pPeriodicSync->IsExtraEqual(pJob))
		{
			delete pPeriodicSync;
			pPeriodicSync = NULL;
			itr = (pCapabilityInfo->periodicSyncList).erase(itr);
			hasChanged = true;
			// While removing an entry from periodic sync array, also remove corresponding entry from sync status list
			if (pSyncStatusInfo)
			{
				pSyncStatusInfo->RemovePeriodicSyncTime(index);
			}
			else
			{
				LOG_LOGD("Could not find corresponding sync status entry");
			}
		}
		else
		{
			index++;
			itr++;
		}
	}

	if (!hasChanged)
	{
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}

	LOG_LOGD("RemovePeriodicSyncJob calling WriteAccountData");
	WriteAccountData();
	WriteStatusData();
	pthread_mutex_unlock(&__capabilityInfoMutex);

	SyncManager::GetInstance()->AlertForChange();
}


pair<CapabilityInfo*, SyncStatusInfo*>
RepositoryEngine::CreateCopyOfCapabilityAndSyncStatusPairN(CapabilityInfo* pCapabilityInfo)
{
	SyncStatusInfo* pSyncStatusInfo = GetOrCreateSyncStatusLocked(pCapabilityInfo->id);
	return make_pair(new CapabilityInfo(*pCapabilityInfo), new SyncStatusInfo(*pSyncStatusInfo));
}


vector<pair<CapabilityInfo*, SyncStatusInfo*> >
RepositoryEngine::GetCopyOfAllCapabilityAndSyncStatusN(void)
{
	pthread_mutex_lock(&__capabilityInfoMutex);

	vector<pair<CapabilityInfo*, SyncStatusInfo*> > allCapabilitiesAndStatusInfo;
	map<int, CapabilityInfo*>::iterator it;
	for (it = __capabilities.begin(); it != __capabilities.end(); it++)
	{
		CapabilityInfo* pInfo = it->second;
		if (pInfo == NULL)
		{
			LOG_LOGD("capability info shouldn't be null");
			continue;
		}
		allCapabilitiesAndStatusInfo.push_back(CreateCopyOfCapabilityAndSyncStatusPairN(pInfo));
	}

	pthread_mutex_unlock(&__capabilityInfoMutex);
	return allCapabilitiesAndStatusInfo;
}


bool
RepositoryEngine::GetSyncAutomatically(account_h account, string capability)
{
	pthread_mutex_lock(&__capabilityInfoMutex);

	if (account)
	{
		CapabilityInfo* pCapabilityInfo = GetCapabilityLocked(account, capability, "GetSyncAutomatically");
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return (pCapabilityInfo && pCapabilityInfo->isEnabled);
	}

	map<int, CapabilityInfo*>::iterator it;
	for (it = __capabilities.begin(); it != __capabilities.end(); it++)
	{
		CapabilityInfo* pCapabilityInfo = (it->second);
		if (pCapabilityInfo->capability.compare(capability) && pCapabilityInfo->isEnabled)
		{
			pthread_mutex_unlock(&__capabilityInfoMutex);
			return true;
		}
	}
	pthread_mutex_unlock(&__capabilityInfoMutex);
	return false;
}


int
RepositoryEngine::GetSyncable(account_h account, string capability)
{
   // int error = pthread_mutex_trylock(__pMutex);
   /* if (error == EBUSY)
	{
		LOG_LOGD("failed to get the lock because another thread holds lock");
	} else if (error == EOWNERDEAD)
	{
		LOG_LOGD(" got the lock, but the critical section state may not be consistent");

	} else {
		switch (error) {
		case EAGAIN:
			LOG_LOGD("err 1");
			break;
		case EINVAL:
			LOG_LOGD("err 2");
			break;
		case ENOTRECOVERABLE:

			LOG_LOGD("err 3");
			break;
		default:

			LOG_LOGD("err 4");
			break;
		}
	}*/
	pthread_mutex_lock(&__capabilityInfoMutex);
	if (account)
	{
		CapabilityInfo* pCapabilityInfo = GetCapabilityLocked(account, capability, "GetSyncable");
		if (!pCapabilityInfo)
		{
			pthread_mutex_unlock(&__capabilityInfoMutex);
			return -1;
		}

		pthread_mutex_unlock(&__capabilityInfoMutex);
		return pCapabilityInfo->syncable;
	}
	map<int, CapabilityInfo*>::iterator it;
	for (it = __capabilities.begin(); it != __capabilities.end(); it++)
	{
		CapabilityInfo* pCapabilityInfo = (it->second);
		if (pCapabilityInfo->capability.compare(capability))
		{
			pthread_mutex_unlock(&__capabilityInfoMutex);
			return pCapabilityInfo->syncable;
		}
	}
	pthread_mutex_unlock(&__capabilityInfoMutex);
	return -1;
}


void
RepositoryEngine::SetSyncable(string appId, account_h account, string capability, int syncable)
{
	if (syncable > 1)
	{
		syncable = 1;
	}
	else if (syncable < -1)
	{
		syncable = -1;
	}

	pthread_mutex_lock(&__capabilityInfoMutex);

	CapabilityInfo* pCapabilityInfo= GetOrCreateCapabilityLocked(appId, account, capability, -1, false);
	if (!pCapabilityInfo)
	{
		LOG_LOGD("GetOrCreateCapabilityLocked returned NULL");
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}
	if (pCapabilityInfo->syncable == syncable)
	{
		LOG_LOGD("SetSyncable already set to %d", syncable);
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}
	pCapabilityInfo->syncable = syncable;
	WriteAccountData();
	pthread_mutex_unlock(&__capabilityInfoMutex);

	if (syncable > 0)
	{
		//TODO RequestSync(), don't know what is happening here
	}
	//TODO reportChange(ContentResolver.SYNC_OBSERVER_TYPE_SETTINGS);
}


void
RepositoryEngine::SetPeriodicSyncTime(int capabilityId, PeriodicSyncJob* pJob , long long when)
{
	if (pJob == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return;
	}

	pthread_mutex_lock(&__capabilityInfoMutex);

	CapabilityInfo* pCapabilityInfo = NULL;
	map<int, CapabilityInfo*>::iterator it = __capabilities.find(capabilityId);
	if (it != __capabilities.end())
	{
		pCapabilityInfo = it->second;
	}

	if (!pCapabilityInfo)
	{
		LOG_LOGD("Invalid capability id, Failed to set periodic sync time");
		pthread_mutex_unlock(&__capabilityInfoMutex);
		return;
	}

	bool found = false;
	for (unsigned int i = 0; i < pCapabilityInfo->periodicSyncList.size(); i++)
	{
		PeriodicSyncJob* pJobToCompare = pCapabilityInfo->periodicSyncList[i];
		if (*pJob == *pJobToCompare)
		{
			__syncStatus[capabilityId]->SetPeriodicSyncTime(i, when);
			found = true;
			break;
		}
	}

	if (!found)
	{
		LOG_LOGD("PeriodicSync does not exist, ignoring SetPeriodicSyncTime, capability [%s]", pCapabilityInfo->capability.c_str());
	}
	pthread_mutex_unlock(&__capabilityInfoMutex);
}
//}//_SyncManager
