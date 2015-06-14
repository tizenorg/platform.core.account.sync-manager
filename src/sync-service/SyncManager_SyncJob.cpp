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
 * @file	SyncManager_SyncJob.cpp
 * @brief	This is the implementation file for the SyncJob class.
 */

#include <string>
#include <sys/time.h>
#include <sstream>
#include <account.h>
#include "sync-log.h"
#include "sync_manager.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncJob.h"

/*namespace _SyncManager
{*/

#ifndef MAX
#define MAX(a, b) a>b?a:b
#endif

extern "C"
{

SyncJob::SyncJob(void)
{
	//Empty
}


SyncJob::~SyncJob(void)
{
	if (pExtras)
	{
		bundle_free(pExtras);
	}
	//TODO uncomment below line while implementing pending sync job list
	/*delete pPendingJob;*/
}


SyncJob::SyncJob(const SyncJob& job)
{
	this->appId = job.appId;
	this->account = job.account;
	int id, id2;
	if (job.account != NULL)
	{
		int ret = account_get_account_id(job.account, &id);
		if (ret != 0)
		{
			LOG_LOGD("Failed to get account id");
			return;
		}
		if (this->account == 0)
		{
			LOG_LOGD("acc null");
		}
		ret = account_get_account_id(this->account, &id2);
		if (ret != 0)
		{
			LOG_LOGD("Failed to get account id2");
			return;
		}
	}
	this->capability = job.capability.c_str();
	this->pExtras = bundle_dup(job.pExtras);
	this->reason = job.reason;
	this->syncSource = job.syncSource;
	this->isParallelSyncAllowed = job.isParallelSyncAllowed;
	this->backoff = job.backoff;
	this->delayUntil = job.delayUntil;
	this->isExpedited = job.isExpedited;
	this->latestRunTime = job.latestRunTime;
	this->effectiveRunTime = job.effectiveRunTime;
	this->flexTime = job.flexTime;
	this->key = job.key.c_str();
}


SyncJob&
SyncJob::operator = (const SyncJob& job)
{
	this->appId = job.appId;
	LOG_LOGD("appId %s", this->appId.c_str());
	this->account = job.account;
	this->capability = job.capability;
	this->pExtras = bundle_dup(job.pExtras);
	this->reason = job.reason;
	this->syncSource = job.syncSource;
	this->isParallelSyncAllowed = job.isParallelSyncAllowed;
	this->backoff = job.backoff;
	this->delayUntil = job.delayUntil;
	this->isExpedited = job.isExpedited;
	this->latestRunTime = job.latestRunTime;
	this->effectiveRunTime = job.effectiveRunTime;
	this->flexTime = job.flexTime;
	this->key = job.key;

	return *this;
}


void
SyncJob::CleanBundle(bundle* pData)
{
	RemoveFalseExtra(pData, "SYNC_OPTION_UPLOAD");//TODO provide these as enum
	RemoveFalseExtra(pData, "SYNC_OPTION_IGNORE_SETTINGS");
	RemoveFalseExtra(pData, "SYNC_OPTION_IGNORE_BACKOFF");
	RemoveFalseExtra(pData, SYNC_OPTION_NO_RETRY);
	RemoveFalseExtra(pData, "SYNC_OPTION_DISCARD_LOCAL_DELETIONS");
	RemoveFalseExtra(pData, "SYNC_OPTION_EXPEDITED");
	RemoveFalseExtra(pData, "SYNC_OPTION_OVERRIDE_TOO_MANY_DELETIONS");
	RemoveFalseExtra(pData, "SYNC_OPTION_DISALLOW_METERED");

	// Remove Config data.
	bundle_del(pData, "SYNC_OPTION_EXPECTED_UPLOAD");
	bundle_del(pData, "SYNC_OPTION_EXPECTED_DOWNLOAD");
}


SyncJob::SyncJob(const string appId, account_h account, const string capability, bundle* pExtras, SyncReason reason, SyncSource source,
		long runTimeFromNow, long flexTime, long backoff, long delayUntil, bool isParallelSyncsAllowed)
{
	this->appId = appId;
	this->account = account;
	this->capability = capability;
	this->pExtras = bundle_dup(pExtras);
	//CleanBundle(this->pExtras);
	this->reason = reason;
	this->syncSource = source;
	this->isParallelSyncAllowed = isParallelSyncsAllowed;
	this->backoff = backoff;
	this->delayUntil = delayUntil;

	long long elapsedTime = SyncManager::GetInstance()->GetElapsedTime();
	if (runTimeFromNow < 0 || IsExpedited())
	{
		this->isExpedited = true;
		this->latestRunTime = elapsedTime;
		this->flexTime = 0;
	}
	else
	{
		this->isExpedited = false;
		this->latestRunTime = runTimeFromNow;
		this->flexTime = flexTime;
	}
	/// UpdateEffectiveRunTime();
	effectiveRunTime = runTimeFromNow;
	this->key = ToKey();
}


bool
SyncJob::GetBundleVal(const char* pVal)
{
	if (pVal == NULL)
	{
		return false;
	}
	else return strcmp(pVal, "true")? true: false;
}


void
SyncJob::RemoveFalseExtra(bundle* pData, const char* pExtra)
{
	if (bundle_get_val(pData, pExtra) ==  false)
	{
		bundle_del(pData, pExtra);
	}
}

bool
SyncJob::IsNoTooManyRetry(void)
{
	const char* pVal = bundle_get_val(pExtras, "SYNC_OPTION_OVERRIDE_TOO_MANY_DELETIONS");
	bool isNoRetry = GetBundleVal(pVal);
	pVal = NULL;
	return isNoRetry;
}

bool
SyncJob::IsNoRetry(void)
{
	const char* pVal = bundle_get_val(pExtras, SYNC_OPTION_NO_RETRY);
	bool isNoTooManyRetry = GetBundleVal(pVal);
	pVal = NULL;
	return isNoTooManyRetry;
}

bool
SyncJob::IsMeteredDisallowed(void)
{
	const char* pVal = bundle_get_val(pExtras, "SYNC_OPTION_DISALLOW_METERED");
	bool isMeteredDisallowed = GetBundleVal(pVal);
	pVal = NULL;
	return isMeteredDisallowed;
}


bool
SyncJob::IsInitialized(void)
{
	const char* pVal = bundle_get_val(pExtras, "SYNC_OPTION_INITIALIZE");
	bool isInitialized = GetBundleVal(pVal);
	pVal = NULL;
	return isInitialized;
}

bool
SyncJob::IsExpedited(void)
{
	const char* pVal = bundle_get_val(pExtras, SYNC_OPTION_EXPEDITED);
	bool isExpedited = GetBundleVal(pVal);
	pVal = NULL;
	return isExpedited;
}

bool
SyncJob::IgnoreBackoff(void)
{
	const char* pVal = bundle_get_val(pExtras, "SYNC_OPTION_IGNORE_BACKOFF");
	bool ignoreBackoff = GetBundleVal(pVal);
	pVal = NULL;
	return ignoreBackoff;
}

void
SyncJob::SetJobExtraValue(const char* data, bool val)
{
	if (val)
	{
		bundle_add(pExtras, data, "true");
	}
	else
	{
		bundle_add(pExtras, data, "false");
	}
}

void
SyncJob::UpdateEffectiveRunTime(void)
{
	effectiveRunTime = IgnoreBackoff() ? latestRunTime : MAX(MAX(latestRunTime, delayUntil), backoff);
}


string
SyncJob::ToKey(void)
{
	LOG_LOGD("Generating key");

	int ret;
	string key;
	char* pName;
	int id;
	stringstream ss;

	if (account != NULL)
	{
		ret = account_get_user_name(account, &pName);

		ret = account_get_account_id(account, &id);

		ss<<id;
		key.append("id:").append(ss.str()).append("name:").append(pName).append("capability:").append(capability);
	}
	else
	{
		key.append("id:").append(appId);
	}
	LOG_LOGD("%s", key.c_str());
	return key;
}


static void
bndl_iterator(const char* pKey, const char* pVal, void* pData)
{
	string str = *((string*)pData);
	str.append(pKey).append("=").append(pVal).append(" ");
}


string
SyncJob::GetExtrasInfo(bundle* pData)
{
	string str;
	if (pData == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return str;
	}
	str.append("[");
	bundle_iterate(pData, bndl_iterator, &str);
	str.append("]");
	return str;
}


/*
 * This compare is based on earliest effective runtime
 */
int
SyncJob::Compare(void* pObj)
{
	SyncJob* pOtherJob = (SyncJob*)pObj;
	if (isExpedited != pOtherJob->isExpedited)
	{
		return isExpedited ? -1 : 1;
	}

	long thisStart = MAX(effectiveRunTime - flexTime, 0);
	long otherStart = MAX(pOtherJob->effectiveRunTime - pOtherJob->flexTime, 0);

	if (thisStart < otherStart)
	{
		return -1;
	}
	else if (thisStart > otherStart)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


}
//}//_SyncManager
