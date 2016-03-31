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
#include "SyncManager_ISyncJob.h"
#include "SyncManager_SyncDefines.h"

/*namespace _SyncManager
{
*/

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;

class SyncJob
		: public ISyncJob
{
public:
	~SyncJob(void);

	SyncJob(const SyncJob &job);

	SyncJob &operator = (const SyncJob &job);

	SyncJob(const string appId, const string syncJobName, int accountId, bundle *pUserData, int syncOption, int syncJobId, SyncType type);

	void Reset(int accountId, bundle* pUserData, int syncOption);

	bool IsExpedited(void);

	bool IsNoRetry(void);

	bool IsNoTooManyRetry(void);

	string GetExtrasInfo(bundle* pData);

	void SetJobExtraValue(const char* data, bool val);

	void IncrementWaitCounter();

	virtual SyncType GetSyncType() {
		return __syncType;
	}

public:
	string __appId;
	string __syncJobName;
	int __accountId;
	bundle* __pExtras;
	bool __isExpedited;
	bool __noRetry;
	/* SyncType __syncType; */
	/* PendingJob* pPendingJob; */
	SyncReason __reason;
	SyncSource __syncSource;
	string __key;
	int __waitCounter;

private:
	void CleanBundle(bundle* Bundle);

	string ToKey(void);

	bool GetBundleVal(const char* pKey);

	void RemoveFalseExtra(bundle* pBundle, const char* pExtraName);
};

#ifdef __cplusplus
}
#endif

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_JOB_H */
