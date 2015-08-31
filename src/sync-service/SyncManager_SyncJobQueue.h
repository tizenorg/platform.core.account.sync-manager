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
 * @file	SyncManager_SyncJobQueue.h
 * @brief	This is the header file for the SyncJobQueue class.
 */

#ifndef SYNC_SERVICE_SYNC_JOB_QUEUE_H
#define SYNC_SERVICE_SYNC_JOB_QUEUE_H

#include <map>
#include <list>
#include <bundle.h>
#include <account.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_SyncJob.h"
#include "SyncManager_PendingJob.h"


/*namespace _SyncManager
{
*/

#ifdef __cplusplus
extern "C"{
#endif

using namespace std;

class SyncJob;

class SyncJobQueue
{
public:

	SyncJobQueue(void);

	~SyncJobQueue(void);

	SyncJobQueue(RepositoryEngine* pSyncRepositoryEngine);

	map<const string, SyncJob*> GetSyncJobQueue(void) const;

	//TODO Uncomment the below code when enabling pending job logic
	//void AddPendingJobs(string appId);

	int AddSyncJob(SyncJob pJob);

	void RemoveSyncJobsForApp(string appId);

	int RemoveSyncJob(string key);

	void RemoveSyncJob(string appId, account_h account, const string capability);

	void OnBackoffChanged(account_h account, const string capability, long backoff);

	void OnDelayUntilTimeChanged(account_h account, const string capability, long delayUntil);

private:
	int AddSyncJob(SyncJob pJob, PendingJob* pPendingJob);

private:
	RepositoryEngine* __pSyncRepositoryEngine;
	map<const string, SyncJob*> __syncJobsList;
};

#ifdef __cplusplus
}
#endif
//}//_SyncManager
#endif//SYNC_SERVICE_SYNC_JOB_QUEUE_H
