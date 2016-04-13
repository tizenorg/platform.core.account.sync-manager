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
 * @file	SyncManager_ManageIdleState.cpp
 * @brief	This is the implementation file for the ManageIdleState class.
 */

#include <stdlib.h>
#include "SyncManager_ManageIdleState.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncManager.h"
#include "sync-log.h"

#define SYNC_TERM_SEC 10

/*namespace _SyncManager
{*/


ManageIdleState::ManageIdleState(void) {
	__termTimerId = -1;
}


/* LCOV_EXCL_START */
ManageIdleState::~ManageIdleState(void) {
}
/* LCOV_EXCL_STOP */


void terminate_service(void);

int
ManageIdleState::OnTermTimerExpired(gpointer) {
	LOG_LOGD("Sync service auto-termination timer is expired");
	terminate_service();

	return 0;
}


void
ManageIdleState::SetTermTimerId(long timerId) {
	__termTimerId = timerId;
}


long
ManageIdleState::GetTermTimerId() const {
	return __termTimerId;
}


void
ManageIdleState::SetTermTimer() {
	if (GetTermTimerId() == -1) {
		guint termTimer = SYNC_TERM_SEC * 1000;
		SetTermTimerId(g_timeout_add(termTimer, ManageIdleState::OnTermTimerExpired, NULL));
		LOG_LOGD("Sync service auto-termination timer is Set as [%d] sec", SYNC_TERM_SEC);
	} else {
		ResetTermTimer();
	}
}


void
ManageIdleState::UnsetTermTimer() {
	if (GetTermTimerId() != -1) {
		LOG_LOGD("Sync service auto-termination timer is Unset");
		g_source_remove((guint)GetTermTimerId());
		SetTermTimerId((long)-1);
	}
}


void
ManageIdleState::ResetTermTimer() {
	if (GetTermTimerId() != -1) {
		LOG_LOGD("Sync service auto-termination timer is Reset");
		g_source_remove((guint)GetTermTimerId());
		SetTermTimerId((long)-1);
	}
	SetTermTimer();
}

