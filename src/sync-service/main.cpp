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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bundle.h>
#include <vector>
#include <app.h>
#include <Elementary.h>
#include "pthread.h"

#include "SyncManager_ServiceInterface.h"
#include "sync-log.h"

extern bool ShutdownInitiated;

static Eina_Bool
OnIdle(void* pUserData)
{
	LOG_LOGD("MainLoop OnIdle");
	return  ECORE_CALLBACK_CANCEL;
}


Eina_Bool
OnTerminate(void *data, int ev_type, void *ev)
{
	LOG_LOGD("MainLoop OnTerminate");

	if (ShutdownInitiated == false) {
		ShutdownInitiated = true;
		sync_service_finalise();
	}

	ecore_main_loop_quit();

	return EINA_TRUE;
}


int
main(int argc, char **argv)
{
	if (!ecore_init()) {
		return -1;
	}
	LOG_LOGD("Sync Service");

	ecore_idler_add(OnIdle, NULL);

	ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, OnTerminate, NULL);

	int ret = sync_service_initialise();
	if (ret != 0) {
		LOG_LOGD("Could not initialise sync service");
		return 0;
	}

	ecore_main_loop_begin();

	ecore_shutdown();

	return 0;
}
