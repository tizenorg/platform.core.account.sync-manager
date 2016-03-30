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
 * @file	SyncManager_CurrentSyncContext.cpp
 * @brief	This is the implementation file for the CurrentSyncContext class.
 */

#include <stdlib.h>
#include "SyncManager_CurrentSyncContext.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncManager.h"
#include "sync-log.h"


/*namespace _SyncManager
{*/


//LCOV_EXCL_START
CurrentSyncContext::CurrentSyncContext(SyncJob* pSyncJob)
{
	__pCurrentSyncJob = pSyncJob;
	__startTime = 0;
	__timerId = -1;
}


CurrentSyncContext::CurrentSyncContext(const CurrentSyncContext& currContext)
{
	__startTime = currContext.GetStartTime();
	__timerId = currContext.GetTimerId();
	__pCurrentSyncJob = currContext.GetSyncJob();
	LOG_LOGE_VOID(__pCurrentSyncJob, "Failed to construct SyncJob in CurrentSyncContext");
}


CurrentSyncContext::~CurrentSyncContext(void)
{
}


SyncJob*
CurrentSyncContext::GetSyncJob() const
{
	return __pCurrentSyncJob;
}


long
CurrentSyncContext::GetStartTime() const
{
	return __startTime;
}


long
CurrentSyncContext::GetTimerId() const
{
	return __timerId;
}


void
CurrentSyncContext::SetTimerId(long id)
{
	__timerId  = id;
}
//LCOV_EXCL_STOP
