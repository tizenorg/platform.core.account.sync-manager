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
 * @file	SyncManager_CurrentSyncContext.h
 * @brief	This is the header file for the CurrentSyncContext class.
 */

#ifndef SYNC_SERVICE_CURRENT_SYNC_CONTEXT_H
#define SYNC_SERVICE_CURRENT_SYNC_CONTEXT_H

#include <pthread.h>
#include "SyncManager_SyncJob.h"
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_CurrentSyncJobQueue.h"

/*namespace _SyncManager
{
*/

class CurrentSyncContext
{
public:
	CurrentSyncContext(SyncJob *pSyncJob);
	CurrentSyncContext(const CurrentSyncContext &job);
	~CurrentSyncContext(void);
	SyncJob *GetSyncJob() const;
	long GetStartTime() const;
	long GetTimerId() const;
	void SetTimerId(long id);


private:
	SyncJob* __pCurrentSyncJob;
	long __startTime;
	long __timerId;

	friend class SyncJobDispatcher;
	friend class CurrentSyncJobQueue;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_CURRENT_SYNC_CONTEXT_H */
