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
 * @file	SyncManager_CurrentSyncJobQueue.h
 * @brief	This is the header file for the CurrentSyncJobQueue class.
 */

#ifndef SYNC_SERVICE_CURRENT_SYNC_JOB_QUEUE_H
#define SYNC_SERVICE_CURRENT_SYNC_JOB_QUEUE_H

#include <string>
#include <account.h>
#include <bundle.h>
#include <map>
#include <queue>
#include "SyncManager_CurrentSyncContext.h"

/*namespace _SyncManager
{
*/

using namespace std;

class CurrentSyncJobQueue
{
public:
	CurrentSyncJobQueue(void);

	~CurrentSyncJobQueue(void);

	int AddSyncJobToCurrentSyncQueue(SyncJob* syncJob);

	bool IsJobActive(CurrentSyncContext *pCurrSync);

	int RemoveSyncContextFromCurrentSyncQueue(CurrentSyncContext* pSyncContext);

	CurrentSyncContext* DoesAccAuthExist(account_h account, string auth);

	list<CurrentSyncContext*> GetOperations(void);

	static string ToKey(account_h account, string auth);

	CurrentSyncContext* GetCurrJobfromKey(string key);

	static int OnTimerExpired(void* data);

private:
	map<const string, CurrentSyncContext*> __currentSyncJobQueue;
	priority_queue<string> __name;
};

//}//_SyncManager
#endif // SYNC_SERVICE_CURRENT_SYNC_JOB_QUEUE_H
