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
 * @file	SyncManager_SyncJobDispatcher.cpp
 * @brief	This is the implementation file for the SyncJobDispatcher class.
 */

#include <sys/time.h>
#include <climits>
#include <alarm.h>
#include <glib.h>
#include <unistd.h>
#include "sync-error.h"
#include "sync-log.h"
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncJob.h"

#ifndef MAX
#define MAX(a, b) a > b? a : b
#endif
#ifndef MIN
#define MIN(a, b) a < b? a : b
#endif

#define NON_PRIORITY_SYNC_WAIT_LIMIT 5

long MAX_TIME_PER_SYNC = 5*60*1000; //5 minutes
long MAX_SIMULTANEOUS_INITIALIZATION_SYNCS = 2;
long MAX_SIMULTANEOUS_REGULAR_SYNCS = 10;
long MAX_TIMEOUT_VAL = 2*60*60*1000; // 2 hours
long MIN_TIMEOUT_VAL = 30*1000; // 30 seconds

long long scheduledTimeoutTime = -1;
alarm_id_t alarm_id = 0;

using namespace std;
/*namespace _SyncManager
{*/

SyncJobDispatcher::SyncJobDispatcher(void)
{
}


SyncJobDispatcher::~SyncJobDispatcher(void)
{
}


int
SyncJobDispatcher::DispatchSyncJob(SyncJob* syncJob)
{
	int ret = SYNC_ERROR_NONE;
	LOG_LOGD("Dispatching sync job [%s], [%s]", syncJob->__appId.c_str(), syncJob->__syncJobName.c_str());

	bool isDataSync = (syncJob->GetSyncType() == SYNC_TYPE_DATA_CHANGE);
	ret = SyncService::GetInstance()->TriggerStartSync(syncJob->__appId.c_str(), syncJob->__accountId, syncJob->__syncJobName.c_str(), isDataSync, syncJob->__pExtras);
	SYNC_LOGE_RET_RES(ret == SYNC_ERROR_NONE, ret, "Failed to start sync job")

	if (SyncManager::GetInstance()->__pCurrentSyncJobQueue) {
		pthread_mutex_lock(&(SyncManager::GetInstance()->__currJobQueueMutex));
		LOG_LOGD("Add to Active Sync queue");

		SyncManager::GetInstance()->__pCurrentSyncJobQueue->AddSyncJobToCurrentSyncQueue(syncJob);
		pthread_mutex_unlock(&(SyncManager::GetInstance()->__currJobQueueMutex));
	}

	return SYNC_ERROR_NONE;
}


void
SyncJobDispatcher::HandleJobCompletedOrCancelledLocked(SyncStatus res, SyncJob *pJob)
{
	LOG_LOGD("Starts");

	switch (res) {
	case SYNC_STATUS_SUCCESS:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_SUCCESS");
		break;

	case SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS");
		SyncManager::GetInstance()->TryToRescheduleJob(res, pJob);
		break;

	case SYNC_STATUS_FAILURE:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_FAILURE");
		SyncManager::GetInstance()->TryToRescheduleJob(res, pJob);
		break;

	case SYNC_STATUS_CANCELLED:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_CANCELLED");
		SyncService::GetInstance()->TriggerStopSync(pJob->__appId.c_str(), pJob->__accountId, pJob->__syncJobName.c_str(), (pJob->GetSyncType() == SYNC_TYPE_DATA_CHANGE), pJob->__pExtras);
		delete pJob;
		break;

	default:
		break;
	}
}


void
SyncJobDispatcher::OnEventReceived(Message msg)
{
	LOG_LOGD("0. Sync Job dispatcher starts");

	if (!SyncManager::GetInstance()->__isSyncPermitted) {
		LOG_LOGD("Sync not permitted now");
		return;
	}

	switch (msg.type) {
		case SYNC_CANCEL: {
				LOG_LOGD("1. Handle Event : SYNC_CANCEL");
				HandleJobCompletedOrCancelledLocked(SYNC_STATUS_CANCELLED, msg.pSyncJob);
				LOG_LOGD("2. Start next Syncjob from main queue");
				TryStartingNextJobLocked();
			}
			break;

		case SYNC_FINISHED: {
				LOG_LOGD("1. Handle Event : SYNC_FINISHED");
				HandleJobCompletedOrCancelledLocked(msg.res, msg.pSyncJob);
				LOG_LOGD("2. Start next Sync job from main queue");
				TryStartingNextJobLocked();
			}
			break;

		case SYNC_CHECK_ALARM: {
				LOG_LOGD("1. Handle Event : SYNC_CHECK_ALARM");
				LOG_LOGD("2. Start next Sync job from main queue");
				TryStartingNextJobLocked();
			}
			break;

		case SYNC_ALARM: {
				LOG_LOGD("1. Handle Event : SYNC_ALARM");
				LOG_LOGD("2. Start next Sync job from main queue");
				TryStartingNextJobLocked();
			}
			break;
		default:
			break;
	};

	LOG_LOGD("3. Sync Job dispatcher Ends");
}


bool
sortFunc(const SyncJob* pJob1, const SyncJob* pJob2)
{
	return false;
}


void
SyncJobDispatcher::TryStartingNextJobLocked()
{
	if (SyncManager::GetInstance()->__isWifiConnectionPresent == false && SyncManager::GetInstance()->__isSimDataConnectionPresent == false) {
//		LOG_LOGD("No network available: Skipping sync");
//		return;
	}

	if (!SyncManager::GetInstance()->__isSyncPermitted) {
		LOG_LOGD("Sync not permitted now: Skipping sync");
		return;
	}

	if (SyncManager::GetInstance()->__isUPSModeEnabled) {
		LOG_LOGD("UPS mode enabled: Skipping sync");
		return;
	}

	if (SyncManager::GetInstance()->__isStorageLow) {
		LOG_LOGD("Storage Low: Skipping sync");
		return;
	}

	pthread_mutex_lock(&(SyncManager::GetInstance()->__syncJobQueueMutex));

	SyncJobQueue* pSyncJobQueue = SyncManager::GetInstance()->GetSyncJobQueue();
	if (pSyncJobQueue == NULL) {
		LOG_LOGD("pSyncJobQueue is null");
		return;
	}

	list< SyncJob* >& jobQueue = pSyncJobQueue->GetSyncJobQueue();
	if (jobQueue.empty()) {
		LOG_LOGD("jobQueue is empty");
	}

	list< SyncJob* >& priorityJobQueue = pSyncJobQueue->GetPrioritySyncJobQueue();
	if (priorityJobQueue.empty()) {
		LOG_LOGD("priorityJobQueue is empty");
	}

	if (jobQueue.empty() && priorityJobQueue.empty()) {
		LOG_LOGD("SyncJob Queues are empty");
		pthread_mutex_unlock(&(SyncManager::GetInstance()->__syncJobQueueMutex));
		return;
	}

	SyncJob* syncJobToRun = NULL;

	if (!jobQueue.empty()) {
		LOG_LOGD("jobQueue is filled");
		SyncJob* nonPrioritySyncJob = jobQueue.front();
		if (nonPrioritySyncJob->__waitCounter > NON_PRIORITY_SYNC_WAIT_LIMIT) {
			LOG_LOGD("Long waiting Non priority job found. Handle this job first");
			syncJobToRun = nonPrioritySyncJob;
			jobQueue.pop_front();
		}
	}

	if (syncJobToRun == NULL && !priorityJobQueue.empty()) {
		LOG_LOGD("Priority job found");
		syncJobToRun = priorityJobQueue.front();
		priorityJobQueue.pop_front();
	}

	if (syncJobToRun == NULL && !jobQueue.empty()) {
		LOG_LOGD("Non priority job found");
		LOG_LOGD("Non priority size: [%d]", jobQueue.size());
		syncJobToRun = jobQueue.front();
		jobQueue.pop_front();
		if (syncJobToRun == NULL)
			LOG_LOGD("syncJobToRun is null");
	}
	pthread_mutex_unlock(&(SyncManager::GetInstance()->__syncJobQueueMutex));

	if (syncJobToRun != NULL) {
		int ret = DispatchSyncJob(syncJobToRun);
		if (ret != SYNC_ERROR_NONE) {
			LOG_LOGD("Failed to dispatch sync job. Adding it back to job queue");
			SyncManager::GetInstance()->ScheduleSyncJob(syncJobToRun, false);
		}
	}
}

//}//_SyncManager
