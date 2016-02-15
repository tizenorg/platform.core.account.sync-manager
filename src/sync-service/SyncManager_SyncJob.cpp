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
#define MAX(a, b) a > b ? a : b
#endif

extern "C" {

SyncJob::~SyncJob(void)
{
	if (__pExtras) {
		bundle_free(__pExtras);
	}
	//TODO uncomment below line while implementing pending sync job list
	/*delete pPendingJob;*/
}


SyncJob::SyncJob(const SyncJob& job)
{
	__appId = job.__appId;
	__accountId = job.__accountId;
	__syncJobName = job.__syncJobName;
	__pExtras = bundle_dup(job.__pExtras);
	__isExpedited = job.__isExpedited;
	__key = job.__key;
	__waitCounter = job.__waitCounter;
	__noRetry = job.__noRetry;
}


SyncJob&
SyncJob::operator = (const SyncJob& job)
{
	__appId = job.__appId;
	__accountId = job.__accountId;
	__syncJobName = job.__syncJobName;
	__pExtras = bundle_dup(job.__pExtras);
	__isExpedited = job.__isExpedited;
	__key = job.__key;
	__waitCounter = job.__waitCounter;
	__noRetry = job.__noRetry;

	return *this;
}


void
SyncJob::CleanBundle(bundle* pData)
{
}


SyncJob::SyncJob(const string appId, const string syncJobName, int account, bundle* pExtras, int syncOption, int syncJobId, SyncType syncType)
		: ISyncJob(syncJobId, syncType)
		, __appId(appId)
		, __syncJobName(syncJobName)
		, __accountId(account)
		, __pExtras(NULL)
		, __isExpedited(syncOption & SYNC_OPTION_EXPEDITED)
		, __noRetry(syncOption & SYNC_OPTION_NO_RETRY)
{
		LOG_LOGD("syncOption: %d", syncOption);
		LOG_LOGD("__isExpedited: %d", __isExpedited);

		if (pExtras) {
			__pExtras = bundle_dup(pExtras);
		}
		__key = ToKey();
}


bool
SyncJob::IsNoRetry(void)
{
	return __noRetry;
}


bool
SyncJob::IsExpedited(void)
{
	return __isExpedited;
}


void
SyncJob::IncrementWaitCounter()
{
	__waitCounter++;
}


string
SyncJob::ToKey(void)
{
	LOG_LOGD("Generating key");

	string key;

	key.append("id:").append(__appId).append(__syncJobName);
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
	if (pData == NULL) {
		LOG_LOGD("Invalid Parameter");
		return str;
	}
	str.append("[");
	bundle_iterate(pData, bndl_iterator, &str);
	str.append("]");
	return str;
}


void
SyncJob::Reset(int accountId, bundle* pUserData, int syncOption)
{
	__accountId = accountId;
	__noRetry = syncOption & SYNC_OPTION_NO_RETRY;
	__isExpedited = syncOption & SYNC_OPTION_EXPEDITED;
	if (__pExtras) {
		bundle_free(__pExtras);
		__pExtras = bundle_dup(pUserData);
	}
}

}

//}//_SyncManager
