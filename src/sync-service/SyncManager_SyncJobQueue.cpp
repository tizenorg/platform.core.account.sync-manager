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
 * @file	SyncManager_SyncJobQueue.cpp
 * @brief	This is the implementation file for the SyncJobQueue class.
 */

#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <dlog.h>
#include <sync-error.h>
#include "sync-log.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncJobQueue.h"

/*namespace _SyncManager
{*/

#ifndef MIN
#define MIN(a, b) a < b ? a : b
#endif

extern "C" {
/* LCOV_EXCL_START */
SyncJobQueue::SyncJobQueue(void)
				: __pSyncRepositoryEngine(NULL) {
	//Empty
}
/* LCOV_EXCL_STOP */


SyncJobQueue::SyncJobQueue(RepositoryEngine* pSyncRepositoryEngine) {
	__pSyncRepositoryEngine = pSyncRepositoryEngine;
}


/* LCOV_EXCL_START */
SyncJobQueue::~SyncJobQueue(void) {
	//Empty
}


list< SyncJob* >&
SyncJobQueue::GetSyncJobQueue(void) {
	return __syncJobsQueue;
}


list< SyncJob* >&
SyncJobQueue::GetPrioritySyncJobQueue(void) {
	return __prioritySyncJobsQueue;
}
/* LCOV_EXCL_STOP */


int
SyncJobQueue::AddSyncJob(SyncJob* pSyncJob) {
	SyncJob* pSyncJobEntry = dynamic_cast< SyncJob* > (pSyncJob);
	SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get sync job");

	if (pSyncJobEntry->IsExpedited()) {
		LOG_LOGD("Priority SyncJob Queue size, before = [%d]", __prioritySyncJobsQueue.size());
		__prioritySyncJobsQueue.push_back(pSyncJob);
		LOG_LOGD("Priority SyncJob Queue size, after = [%d]", __prioritySyncJobsQueue.size());
	} else {
		LOG_LOGD("SyncJob Queue size, before = [%d]", __syncJobsQueue.size());
		__syncJobsQueue.push_back(pSyncJob);
		LOG_LOGD("SyncJob Queue size, after = [%d]", __syncJobsQueue.size());
	}

	return SYNC_ERROR_NONE;
}


int
SyncJobQueue::RemoveSyncJob(SyncJob* pSyncJob) {
	if (pSyncJob ->IsExpedited()) {
		LOG_LOGD("Priority SyncJob Queue size, before = [%d]", __prioritySyncJobsQueue.size());
		__prioritySyncJobsQueue.remove(pSyncJob);
		LOG_LOGD("Priority SyncJob Queue size, after = [%d]", __prioritySyncJobsQueue.size());
	} else {
		LOG_LOGD("SyncJob Queue size, before = [%d]", __syncJobsQueue.size());
		__syncJobsQueue.remove(pSyncJob);
		LOG_LOGD("SyncJob Queue size, after = [%d]", __syncJobsQueue.size());
	}

	return SYNC_ERROR_NONE;
}


/* LCOV_EXCL_START */
void
SyncJobQueue::UpdateAgeCount() {
	list< SyncJob* >::iterator itr = __syncJobsQueue.begin();
	while (itr != __syncJobsQueue.end()) {
		(*itr)->IncrementWaitCounter();
	}
}
/* LCOV_EXCL_STOP */

}
//}//_SyncManager
