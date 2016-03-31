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

#ifndef SYNC_SERVICE_SYNC_WORKER_RESULT_LISTENER_H
#define SYNC_SERVICE_SYNC_WORKER_RESULT_LISTENER_H

#include "SyncManager_SyncDefines.h"

/*namespace _SyncManager
{
*/

class ISyncWorkerResultListener
{
public:
	virtual ~ISyncWorkerResultListener() {};	/* LCOV_EXCL_LINE */

	virtual void OnEventReceived(Message msg) = 0;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_WORKER_RESULT_LISTENER_H */
