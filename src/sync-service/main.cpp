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

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bundle.h>
#include <vector>
#include <app.h>
#include <aul.h>
#include <Elementary.h>
#include "pthread.h"

#include "SyncManager_ServiceInterface.h"
#include "sync-log.h"

#define SYS_DBUS_INTERFACE				"org.tizen.system.deviced.PowerOff"
#define SYS_DBUS_MATCH_RULE				"type='signal',interface='org.tizen.system.deviced.PowerOff'"
#define POWEROFF_MSG					"ChangeState"

static bool ShutdownInitiated = false;

DBusHandlerResult
DbusSignalHandler(DBusConnection* pConnection, DBusMessage* pMsg, void* pUserData)
{
	if (dbus_message_is_signal(pMsg, SYS_DBUS_INTERFACE, POWEROFF_MSG))
	{
		LOG_LOGD("Shutdown dbus received");
		if (ShutdownInitiated == false)
		{
			ShutdownInitiated = true;
			sync_service_finalise();
//			ecore_main_loop_quit();
		}
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


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

	if (ShutdownInitiated == false)
	{
		ShutdownInitiated = true;
		sync_service_finalise();
	}

//	ecore_main_loop_quit();

	return EINA_TRUE;
}


int
main(int argc, char **argv)
{
/*
	if (!ecore_init())
	{
		return -1;
	}
	LOG_LOGD("Sync Service");
*/
//	ecore_idler_add(OnIdle, NULL);

	//Dbus handler to catch shutdown signal in kiran
	DBusError error;

	dbus_error_init(&error);
	DBusConnection* pConn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);

	if (dbus_error_is_set(&error))
	{
		LOG_LOGD("Failed to get System BUS connection: %s", error.message);
		dbus_error_free(&error);
	}
	else
	{
		dbus_connection_setup_with_g_main(pConn, NULL);
		//dbus_bus_get_with_g_main ()
		if (dbus_error_is_set(&error))
		{
			LOG_LOGD("Failed to add D-BUS poweroff match rule, cause: %s", error.message);
			dbus_error_free(&error);
		}
		else
		{
			dbus_bus_add_match(pConn, SYS_DBUS_MATCH_RULE, &error);
			if (dbus_error_is_set(&error))
			{
				LOG_LOGD("Failed to add Poweroff match rule, cause: %s", error.message);
				dbus_error_free(&error);
			}
			else
			{
				if (!dbus_connection_add_filter(pConn, DbusSignalHandler, NULL, NULL))
				{
					LOG_LOGD("Not enough memory to add poweroff filter");
					dbus_bus_remove_match(pConn, SYS_DBUS_MATCH_RULE, NULL);
				}
			}
		}
	}

//	ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, OnTerminate, NULL);

	int ret = sync_service_initialise();
	if (ret != 0)
	{
		LOG_LOGD("Could not initialise sync service");
		return 0;
	}

//	ecore_main_loop_begin();

//	ecore_shutdown();

	return 0;
}
