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
 * @file	SyncManager_ManageIdleState.h
 * @brief	This is the header file for the ManageIdleState class.
 */

#ifndef SYNC_SERVICE_MANAGE_IDLE_STATE_H
#define SYNC_SERVICE_MANAGE_IDLE_STATE_H

#include <glib.h>

/*namespace _SyncManager
{
*/

using namespace std;

class ManageIdleState
{
public:
	ManageIdleState(void);

	~ManageIdleState(void);

	static int OnTermTimerExpired(gpointer);

	void SetTermTimerId(long timerId);

	long GetTermTimerId() const;

	void SetTermTimer(void);

	void UnsetTermTimer(void);

	void ResetTermTimer(void);

private:
	long __termTimerId;
};

//}//_SyncManager
#endif // SYNC_SERVICE_MANAGE_IDLE_STATE_H
