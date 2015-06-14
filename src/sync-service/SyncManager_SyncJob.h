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
 * @file	SyncManager_SyncJob.h
 * @brief	This is the header file for the SyncJob class.
 */


#ifndef SYNC_SERVICE_SYNC_JOB_H
#define SYNC_SERVICE_SYNC_JOB_H

#include <string.h>
#include <bundle.h>
#include <bundle_internal.h>
#include <account.h>
#include <stdio.h>
#include <iostream>
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_SyncDefines.h"

/*namespace _SyncManager
{
*/

#ifdef __cplusplus
extern "C"{
#endif

using namespace std;

class SyncJob
{

public:
	SyncJob(void);

	~SyncJob(void);

	SyncJob(const SyncJob& job);

	SyncJob& operator=(const SyncJob& job);

	SyncJob(const string appId, account_h account, const string capability, bundle* pExtras, SyncReason reason, SyncSource source,
			long runTimeFromNow, long flexTime, long backoff, long delayUntil, bool isParallelSyncsAllowed);

	bool IsMeteredDisallowed(void);

	bool IsInitialized(void);

	bool IsExpedited(void);

	bool IgnoreBackoff(void);

	bool IsNoRetry(void);

	bool IsNoTooManyRetry(void);

	string GetExtrasInfo(bundle* pData);

	void UpdateEffectiveRunTime(void);

	int Compare(void* pOtherJob);

	void SetJobExtraValue(const char* data, bool val);

public:
	string appId;
	account_h account;
	SyncReason reason;
	SyncSource syncSource;
	bundle* pExtras;
	string capability;
	string key;
	bool isParallelSyncAllowed;
	bool isExpedited;
	long latestRunTime; //Elapsed real time in millis at which to run this sync.
	long backoff; ///Set by the SyncManager in order to delay retries.
	long delayUntil; //Specified by the adapter to delay subsequent sync operations.
	long effectiveRunTime; //Elapsed real time in millis when this sync will be run.Depends on max(backoff, latestRunTime, and delayUntil).
	long flexTime; //Amount of time before effectiveRunTime from which this sync can run

	//PendingJob* pPendingJob;

private:

	void CleanBundle(bundle* Bundle);

	string ToKey(void);

	bool GetBundleVal(const char* pKey);

	void RemoveFalseExtra(bundle* pBundle, const char* pExtraName);

};
#ifdef __cplusplus
}
#endif
//}//_SyncManager
#endif//SYNC_SERVICE_SYNC_JOB_H
