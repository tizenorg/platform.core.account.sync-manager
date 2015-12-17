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
#include "SyncManager_SyncJobsAggregator.h"
#include "SyncManager_SyncJobsInfo.h"
#include "sync-log.h"
#include "sync-error.h"


/*namespace _SyncManager
{
*/

#define SYNC_DIRECTORY "sync-manager"
#define SYNC_DATA_DIR tzplatform_mkpath(TZ_USER_DATA, "/sync-manager")
#define PATH_ACCOUNT tzplatform_mkpath(TZ_USER_DATA, "/sync-manager/accounts.xml")
#define PATH_SYNCJOBS tzplatform_mkpath(TZ_USER_DATA, "/sync-manager/syncjobs.xml")
#define PATH_SYNCADAPTERS tzplatform_mkpath(TZ_USER_DATA, "/sync-manager/syncadapters.xml")
#define PATH_STATUS tzplatform_mkpath(TZ_USER_DATA, "/sync-manager/statusinfo.bin")

#ifndef MAX
#define MAX(a, b) a > b ? a : b
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

static const xmlChar XML_NODE_PACKAGE[]							= "package";
static const xmlChar XML_NODE_SYNC_JOB[]						= "job";
static const xmlChar XML_ATTR_JOB_APP_ID[]						= "appId";
static const xmlChar XML_ATTR_JOB_ACCOUNT_ID[]					= "accountId";
static const xmlChar XML_ATTR_JOB_ID[]							= "jobId";
static const xmlChar XML_ATTR_JOB_NAME[]						= "jobName";
static const xmlChar XML_ATTR_JOB_CAPABILITY[]					= "capability";
static const xmlChar XML_ATTR_JOB_OPTION_NORETRY[]				= "noRetry";
static const xmlChar XML_ATTR_JOB_OPTION_EXPEDIT[]				= "expedit";
static const xmlChar XML_ATTR_JOB_TYPE[]						= "syncType";


static const xmlChar XML_NODE_SYNCADAPTERS[]					= "sync-adapters";
static const xmlChar XML_ATTR_COUNT[]							= "count";

static const xmlChar XML_NODE_SYNCADAPTER[]						= "sync-adapter";
static const xmlChar XML_ATTR_SYNCADAPTER_SERVICE_APP_ID[]		= "service-app-id";
static const xmlChar XML_ATTR_PACKAGE_ID[]						= "package-id";

static const string SYNCABLE_STATE_UNKNOWN						= "unknown";

const long RepositoryEngine::NOT_IN_BACKOFF_MODE = -1;
const long RepositoryEngine::DEFAULT_PERIOD_SEC = 24*60*60;	// 1 day
const double RepositoryEngine::DEFAULT_FLEX_PERCENT = 0.04;		// 4 percent
const long RepositoryEngine::DEFAULT_MIN_FLEX_ALLOWED_SEC = 5;	// 5 seconds

#define ID_FOR_ACCOUNT_LESS_SYNC -2

RepositoryEngine* RepositoryEngine::__pInstance = NULL;


RepositoryEngine*
RepositoryEngine::GetInstance(void)
{
	if (!__pInstance) {
		__pInstance = new (std::nothrow) RepositoryEngine();
		if (__pInstance == NULL) {
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
{
	if (pthread_mutex_init(&__capabilityInfoMutex, NULL) != 0) {
		LOG_LOGD("\n __syncJobQueueMutex init failed\n");
		return;
	}
}


void
RepositoryEngine::OnBooting()
{
	ReadSyncAdapters();
	ReadSyncJobsData();
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
RepositoryEngine::ReadSyncJobsData(void)
{
	LOG_LOGD("Read Sync jobs");

	//Parse the Xml file
	const char* pDocName;
	xmlDocPtr doc = NULL;
	xmlNodePtr cur = NULL;

	pDocName = PATH_SYNCJOBS;
	doc = xmlParseFile(pDocName);

	if (doc == NULL) {
		LOG_LOGD("Failed to parse syncjobs.xml.");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		LOG_LOGD("Found empty document while parsing syncjobs.xml.");
		xmlFreeDoc(doc);
		return;
	}

	//Parse sync jobs
	if (xmlStrcmp(cur->name, XML_NODE_JOBS)) {
		LOG_LOGD("Found empty document while parsing syncjobs.xml.");
		xmlFreeDoc(doc);
		return;
	}
	else {
		xmlChar* pTotalJobsCount = xmlGetProp(cur, XML_ATTR_JOBS_COUNT);
		int totalcount = (pTotalJobsCount == NULL) ? 0 : atoi((char*)pTotalJobsCount);
		LOG_LOGD("Total Sync jobs [%d]", totalcount);

		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if (!xmlStrcmp(cur->name, XML_NODE_PACKAGE)) {
				xmlChar* pPackageId;
				xmlChar* pJobsCount;

				pPackageId = xmlGetProp(cur, XML_ATTR_PACKAGE_ID);
				pJobsCount = xmlGetProp(cur, XML_ATTR_JOBS_COUNT);

				int count = (pJobsCount == NULL) ? 0 : atoi((char*)pJobsCount);
				LOG_LOGD("Sync jobs for package [%s] [%d]", pPackageId, count);

				xmlNodePtr curJob = cur->xmlChildrenNode;
				while (curJob != NULL) {
					if (!xmlStrcmp(curJob->name, XML_NODE_SYNC_JOB)) {
						ParseSyncJobsN(curJob, pPackageId);
					}
					curJob = curJob->next;
				}
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

	if (doc == NULL) {
		LOG_LOGD("Failed to parse syncadapters.xml.");
		return;
	}

	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		LOG_LOGD("Found empty document while parsing syncadapters.xml.");
		xmlFreeDoc(doc);
		return;
	}

	//Parse sync jobs
	if (xmlStrcmp(cur->name, XML_NODE_SYNCADAPTERS)) {
		LOG_LOGD("Found empty document while parsing syncadapters.xml.");
		xmlFreeDoc(doc);
		return;
	}
	else {
		xmlChar* pSACount;

		pSACount = xmlGetProp(cur, XML_ATTR_COUNT);
		int count = (pSACount == NULL) ? 0 : atoi((char*)pSACount);

		LOG_LOGD("sync adapter count %d", count);

		SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();

		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if (!xmlStrcmp(cur->name, XML_NODE_SYNCADAPTER)) {
				xmlChar* pServiceAppId = xmlGetProp(cur, XML_ATTR_SYNCADAPTER_SERVICE_APP_ID);
				xmlChar* pPackageId = xmlGetProp(cur, XML_ATTR_PACKAGE_ID);

				pAggregator->AddSyncAdapter((char*)pPackageId, (char*)pServiceAppId);
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
RepositoryEngine::WriteSyncJobsData(void)
{
	LOG_LOGD("Starts");

	xmlDocPtr doc;
	xmlNodePtr rootNode, jobNode;
	stringstream ss;

	doc = xmlNewDoc(_VERSION);

	rootNode = xmlNewNode(NULL, XML_NODE_JOBS);
	xmlDocSetRootElement(doc, rootNode);

	SyncJobsAggregator* pSyncJobsAggregator = SyncManager::GetInstance()->GetSyncJobsAggregator();

	map<string, SyncJobsInfo*>& syncJobs = pSyncJobsAggregator->GetAllSyncJobs();

	ss << syncJobs.size();
	xmlNewProp(rootNode, XML_ATTR_COUNT, (const xmlChar*)ss.str().c_str());
	LOG_LOGD("size %d", syncJobs.size());

	map<string, SyncJobsInfo*>::iterator itr = syncJobs.begin();
	while (itr != syncJobs.end()) {
		string package = itr->first;
		SyncJobsInfo* pJobsInfo = itr->second;

		map<int, ISyncJob*>& packageJobs = pJobsInfo->GetAllSyncJobs();

		xmlNodePtr packageNode = xmlNewChild(rootNode, NULL, XML_NODE_PACKAGE, NULL);
		xmlNewProp(packageNode, XML_ATTR_PACKAGE_ID, (const xmlChar*)(package.c_str()));
		ss.str(string());

		ss << packageJobs.size();
		xmlNewProp(packageNode, XML_ATTR_JOBS_COUNT, (const xmlChar*)ss.str().c_str());
		ss.str(string());

		map<int, ISyncJob*>::iterator it;
		for (it = packageJobs.begin(); it != packageJobs.end(); it++) {
			SyncJob* pJob = dynamic_cast<SyncJob*> (it->second);
			if (pJob == NULL) {
				LOG_LOGD("Invalid sync job entry");
				continue;
			}

			jobNode = xmlNewChild(packageNode, NULL, XML_NODE_SYNC_JOB, NULL);
			xmlNewProp(jobNode, XML_ATTR_JOB_APP_ID, (const xmlChar*)(pJob->__appId.c_str()));

			ss << pJob->__accountId;
			xmlNewProp(jobNode, XML_ATTR_JOB_ACCOUNT_ID, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			ss << (int)pJob->GetSyncJobId();
			xmlNewProp(jobNode, XML_ATTR_JOB_ID, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			ss << (int)pJob->GetSyncType();
			xmlNewProp(jobNode, XML_ATTR_JOB_TYPE, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			ss << (int)pJob->__isExpedited;
			xmlNewProp(jobNode, XML_ATTR_JOB_OPTION_EXPEDIT, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			ss << (int)pJob->__noRetry;
			xmlNewProp(jobNode, XML_ATTR_JOB_OPTION_NORETRY, (const xmlChar*)ss.str().c_str());
			ss.str(string());

			xmlNewProp(jobNode, XML_ATTR_JOB_NAME, (const xmlChar*)pJob->__syncJobName.c_str());
			ss.str(string());

			if (pJob->__pExtras) {
				bundle_iterate(pJob->__pExtras, bndl_iterator, &jobNode);
			}
			if (pJob->GetSyncType() == SYNC_TYPE_PERIODIC) {
				PeriodicSyncJob* pPeriodJob = dynamic_cast<PeriodicSyncJob*> (pJob);
				if (pPeriodJob == NULL) {
					LOG_LOGD("Invalid periodic sync job entry");
					continue;
				}
				ss << (int)pPeriodJob->__period;
				xmlNewProp(jobNode, XML_ATTR_PERIODIC_SYNC_PEIOD, (const xmlChar*)ss.str().c_str());
				ss.str(string());
			}
		}

		itr++;
	}

	int ret = xmlSaveFormatFileEnc(PATH_SYNCJOBS, doc, "UTF-8" , 1);
	if (ret < 0) {
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
	if (!pAggregator) {
		LOG_LOGD("Failed to get sync adapter aggregator, skip writing to file");
		xmlFreeDoc(doc);
		return;
	}

	ss << pAggregator->__syncAdapterList.size();
	xmlNewProp(rootNode, XML_ATTR_COUNT, (const xmlChar*)ss.str().c_str());
	if (pAggregator->__syncAdapterList.size() != 0) {
		ss.str(string());
		for (map<string, string>::iterator it = pAggregator->__syncAdapterList.begin(); it != pAggregator->__syncAdapterList.end(); ++it) {
			string syncAdapter = it->second;
			string packageId = it->first;
			saNode = xmlNewChild(rootNode, NULL, XML_NODE_SYNCADAPTER, NULL);
			xmlNewProp(saNode, XML_ATTR_SYNCADAPTER_SERVICE_APP_ID, (const xmlChar*)(syncAdapter.c_str()));
			xmlNewProp(saNode, XML_ATTR_PACKAGE_ID, (const xmlChar*)(packageId.c_str()));
		}
	}

	int ret = xmlSaveFormatFileEnc(PATH_SYNCADAPTERS, doc, "UTF-8" , 1);
	if (ret < 0) {
		LOG_LOGD("Failed to write sync adapter data, error %d", ret);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();

	LOG_LOGD(" Ends");
}




void
RepositoryEngine::ParseExtras(xmlNodePtr cur, bundle* pExtra)
{
	xmlChar* pKey = xmlGetProp(cur, XML_ATTR_SYNC_EXTRA_KEY);
	xmlChar* pVal = xmlGetProp(cur, XML_ATTR_SYNC_EXTRA_VALUE);

	if (!pKey || !pVal) {
		return;
	}

	bundle_add(pExtra, (char*)pKey, (char*)pVal);
}


void
RepositoryEngine::ParseSyncJobsN(xmlNodePtr cur, xmlChar* pPackage)
{
	//xmlChar* pAppId = xmlGetProp(cur, XML_ATTR_JOB_APP_ID);
	xmlChar* pAccId = xmlGetProp(cur, XML_ATTR_JOB_ACCOUNT_ID);
	xmlChar* pJobId = xmlGetProp(cur, XML_ATTR_JOB_ID);
	xmlChar* pJobName = xmlGetProp(cur, XML_ATTR_JOB_NAME);
	xmlChar* pJobNoRetry = xmlGetProp(cur, XML_ATTR_JOB_OPTION_NORETRY);
	xmlChar* pJobExpedit = xmlGetProp(cur, XML_ATTR_JOB_OPTION_EXPEDIT);
	xmlChar* pJobType = xmlGetProp(cur, XML_ATTR_JOB_TYPE);

	SyncType type = (pJobType == NULL) ? SYNC_TYPE_UNKNOWN : (SyncType)atoi((char*)pJobType);
	bool noretry = (pJobNoRetry == NULL) ? false : atoi((char*)pJobNoRetry);
	bool expedit = (pJobExpedit == NULL) ? false : atoi((char*)pJobExpedit);
	int acountId = (pAccId == NULL) ? -1 : atoi((char*)pAccId);
	int jobId = (pJobId == NULL) ? -1 : atoi((char*)pJobId);
	int syncOption = (noretry) ? 0x00 : 0x02;
	syncOption |= (expedit) ? 0x00 : 0x01;

	bundle* pExtra = NULL;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		pExtra = bundle_create();
		ParseExtras(cur, pExtra);
		cur = cur->next;
	}

	switch (type) {
		case SYNC_TYPE_DATA_CHANGE: {
			SyncManager::GetInstance()->AddDataSyncJob((char*)pPackage, (char*)pJobName, acountId, pExtra, syncOption, jobId, (char*)pJobName);
			break;
		}
		case SYNC_TYPE_ON_DEMAND: {
			SyncManager::GetInstance()->AddOnDemandSync((char*)pPackage, (char*)pJobName, acountId, pExtra, syncOption, jobId);
			break;
		}
		case SYNC_TYPE_PERIODIC: {
			xmlChar* pPeriod = xmlGetProp(cur, XML_ATTR_PERIODIC_SYNC_PEIOD);
			int period = (pPeriod == NULL)? 1800 : atoi((char*) pPeriod);

			SyncManager::GetInstance()->AddPeriodicSyncJob((char*)pPackage, (char*)pJobName, acountId, pExtra, syncOption, jobId, period);
			break;
		}
		case SYNC_TYPE_UNKNOWN:
		default: {
			LOG_LOGD("failed add sync job: sync type is SYNC_TYPE_UNKNOWN");
			break;
		}
	}
}


void
RepositoryEngine::SaveCurrentState(void)
{
	LOG_LOGD("saving states during shutdown");
	pthread_mutex_lock(&__capabilityInfoMutex);
	WriteSyncJobsData();
	pthread_mutex_unlock(&__capabilityInfoMutex);
	WriteSyncAdapters();
}


//}//_SyncManager

