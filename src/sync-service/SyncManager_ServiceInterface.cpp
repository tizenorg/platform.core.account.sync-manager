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

#include "SyncManager_ServiceInterface.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncManager.h"

/*namespace _SyncManager
{*/

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif


API int sync_service_initialise(void) {
	return SyncService::GetInstance()->StartService();
}


API int sync_service_finalise(void) {
	SyncService::GetInstance()->HandleShutdown();
	SyncManager::Destroy();
	SyncService::Destroy();

	LOG_LOGD("Sync Service Terminated");

	return 0;
}

//}//_SyncManager
