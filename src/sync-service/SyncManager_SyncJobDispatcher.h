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
 * @file	SyncManager_SyncJobDispatcher.h
 * @brief	This is the header file for the SyncJobDispatcher class.
 */


#ifndef SYNC_SERVICE_SYNC_JOB_DISPATCHER_H
#define SYNC_SERVICE_SYNC_JOB_DISPATCHER_H

#include "account.h"
#include <alarm.h>
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncJobQueue.h"
#include "SyncManager_SyncWorkerResultListener.h"
#include "SyncManager_Singleton.h"

/*namespace _SyncManager
{
*/

class CurrentSyncContext;

class SyncJobDispatcher
		:public ISyncWorkerResultListener
{
public:

	SyncJobDispatcher(void);

	~SyncJobDispatcher(void);

	int DispatchSyncJob(SyncJob syncJob);

	//ISyncWorkerResultListener
	void OnEventReceived(Message msg);

	static int OnAlarmExpired(alarm_id_t alarm_id, void *user_param);

private:
	SyncJobDispatcher(const SyncJobDispatcher&);

	const SyncJobDispatcher& operator=(const SyncJobDispatcher&);

	void HandleJobCompletedOrCancelledLocked(SyncStatus res, SyncJob *pJob);

	long long TryStartingNextJobLocked();

	long long SchedulePeriodicSyncJobs(void);

 };
//}//_SyncManager
#endif //SYNC_SERVICE_SYNC_JOB_DISPATCHER_H
