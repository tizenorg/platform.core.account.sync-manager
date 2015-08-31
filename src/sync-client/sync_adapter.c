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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdio.h>
#include <app.h>
#include <account.h>
#include <bundle.h>
#include <pthread.h>
#include "sync_adapter.h"
#include "sync-ipc-marshal.h"
#include "sync-manager-stub.h"
#include "sync-adapter-stub.h"
#include "sync-log.h"
#include "sync-error.h"

typedef enum {
	SYNC_STATUS_SUCCESS = 0,
	SYNC_STATUS_CANCELLED =  -1,
	SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS = -2,
	SYNC_STATUS_FAILURE = -3
} SyncSatus;


#define SYNC_THREAD 0		/*As sync adapter itself runs in service app*/

#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_COMMON_DBUS_PATH "/org/tizen/sync/adapter"
#define APP_CONTROL_OPERATION_START_SYNC "http://tizen.org/appcontrol/operation/start_sync"
#define APP_CONTROL_OPERATION_CANCEL_SYNC "http://tizen.org/appcontrol/operation/cancel_sync"


typedef struct sync_adapter_s {
	TizenSyncAdapter *sync_adapter_obj;
	pthread_t cur_thread;
	bool __syncRunning;
	sync_adapter_start_sync_cb start_sync_cb;
	sync_adapter_cancel_sync_cb cancel_sync_cb;
} sync_adapter_s;

static sync_adapter_s *g_sync_adapter = NULL;

extern int read_proc(const char *path, char *buf, int size);
extern char *proc_get_cmdline_self();

typedef struct sync_thread_params_s {
	TizenSyncAdapter *sync_adapter_obj;
	int accountId;
	bundle *extra;
	char *capability;
} sync_thread_params_s;


void *run_sync(void *params)
{
	LOG_LOGD("Sync thread started");
	sync_thread_params_s *sync_params = (sync_thread_params_s *)params;
	if (sync_params == NULL || g_sync_adapter == NULL) {
		LOG_LOGD("sync_params or g_sync_adapter is NULL");
	} else {
		account_h account = NULL;
		int ret = 0;
		if (sync_params->accountId != -1) {
			ret = account_create(&account);
			SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, NULL, "account_create() failed");

			ret =  account_query_account_by_account_id(sync_params->accountId, &account);
			SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, NULL, "account_query_account_by_account_id() failed %d");
		}

		ret = g_sync_adapter->start_sync_cb(account, sync_params->extra, sync_params->capability);

		if (ret == 0)
			tizen_sync_adapter_call_send_result_sync(sync_params->sync_adapter_obj, (int)SYNC_STATUS_SUCCESS, sync_params->accountId, sync_params->capability, NULL, NULL);
		else
			tizen_sync_adapter_call_send_result_sync(sync_params->sync_adapter_obj, (int)SYNC_STATUS_FAILURE, sync_params->accountId, sync_params->capability, NULL, NULL);

		g_sync_adapter->cur_thread = 0;
	}
	free(sync_params);

	return NULL;
}


gboolean
sync_adapter_on_start_sync(
		TizenSyncAdapter *pObject,
		gint accountId,
		GVariant *pBundleArg,
		const gchar *pCapabilityArg)
{
	LOG_LOGD("On start sync called");
	SYNC_LOGE_RET_RES(pObject != NULL, true, "sync adapter object is null");

	if (g_sync_adapter->cur_thread) {
		LOG_LOGD("Sync thread running");
		tizen_sync_adapter_call_send_result_sync(pObject, (int)SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS, accountId, pCapabilityArg, NULL, NULL);

		return true;
	}

	bundle *ext = umarshal_bundle(pBundleArg);
	SYNC_LOGE_RET_RES(ext != NULL, true, "unmarshalling failed");

	sync_thread_params_s *sync_params = (sync_thread_params_s *) malloc(sizeof(struct sync_thread_params_s));
	SYNC_LOGE_RET_RES(sync_params != NULL, true, "Out of memory");

	sync_params->sync_adapter_obj = pObject;
	sync_params->accountId = accountId;
	sync_params->extra = ext;
	sync_params->capability = strdup(pCapabilityArg);

	pthread_t thread;
	int ret = pthread_create(&thread, NULL, run_sync, (void *) sync_params);
	SYNC_LOGE_RET_RES(ret == 0, true, "pthread create failed");

	g_sync_adapter->cur_thread = thread;

	return true;
}


gboolean
sync_adapter_on_stop_sync(
		TizenSyncAdapter *pObject,
		gint accountId,
		const gchar *pCapabilityArg)
{
	LOG_LOGD("handle stop in adapter");
	SYNC_LOGE_RET_RES(pObject != NULL, true, "sync adapter object is null");

	account_h account = NULL;
	int ret = 0;
	if (accountId != -1) {
		ret = account_create(&account);
		SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, true, "account_create() failed");

		ret =  account_query_account_by_account_id(accountId, &account);
		SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, true, "account_query_account_by_account_id() failed");
	}

	if (g_sync_adapter->cur_thread)	{
		 LOG_LOGD("Sync thread active. Cancellig it");
		 pthread_cancel(g_sync_adapter->cur_thread);
	} else {
		 LOG_LOGD("Sync thread not active");
	}

	g_sync_adapter->cur_thread = 0;

	g_sync_adapter->cancel_sync_cb(account, pCapabilityArg);

	return true;
}


int sync_adapter_init(const char *capability)
{
	LOG_LOGD("capability %s", capability);

	bool account_less_sa = true;
	if (capability != NULL) {
		if (!(strcmp(capability, "http://tizen.org/account/capability/calendar")) ||
			!(strcmp(capability, "http://tizen.org/account/capability/contact"))) {
			account_less_sa = false;
		} else {
			LOG_LOGD("sync client: invalid capability");
			return SYNC_ERROR_INVALID_PARAMETER;
		}
	}

	char *command_line = proc_get_cmdline_self();

	GError *error = NULL;
	GDBusConnection *connection = NULL;
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	SYNC_LOGE_RET_RES(connection != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

	TizenSyncManager *ipcObj = tizen_sync_manager_proxy_new_sync(connection,
			G_DBUS_PROXY_FLAGS_NONE,
			"org.tizen.sync",
			"/org/tizen/sync/manager",
			NULL,
			&error);
	SYNC_LOGE_RET_RES(error == NULL && ipcObj != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

	tizen_sync_manager_call_add_sync_adapter_sync(ipcObj, account_less_sa, capability, command_line, NULL, NULL);

	if (!g_sync_adapter) {
		g_sync_adapter = (sync_adapter_s *) malloc(sizeof(struct sync_adapter_s));
		SYNC_LOGE_RET_RES(g_sync_adapter != NULL, SYNC_ERROR_OUT_OF_MEMORY, "Out of memory");

		g_sync_adapter->sync_adapter_obj = NULL;
		g_sync_adapter->cur_thread = 0;
	}

	return SYNC_ERROR_NONE;
}


void sync_adapter_destroy(void)
{
	LOG_LOGD("sync_adapter_destroy");
	if (g_sync_adapter) {
		free(g_sync_adapter);
		g_sync_adapter = NULL;
	}
}


int sync_adapter_set_callbacks(sync_adapter_start_sync_cb on_start_cb, sync_adapter_cancel_sync_cb on_cancel_cb)
{
	SYNC_LOGE_RET_RES(g_sync_adapter != NULL, SYNC_ERROR_SYSTEM, "sync_adapter_init should be called first");

	if (on_start_cb == NULL || on_cancel_cb == NULL)
		return SYNC_ERROR_INVALID_PARAMETER;

	if (g_sync_adapter->sync_adapter_obj) {
		g_sync_adapter->start_sync_cb = on_start_cb;
		g_sync_adapter->cancel_sync_cb = on_cancel_cb;
		return SYNC_ERROR_NONE;
	}

	pid_t pid = getpid();
	GError *error = NULL;
	GDBusConnection *connection = NULL;
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	SYNC_LOGE_RET_RES(connection != NULL, SYNC_ERROR_SYSTEM, "System error occured %s", error->message);

	char obj_path[50];
	snprintf(obj_path, 50, "%s/%d", SYNC_ADAPTER_COMMON_DBUS_PATH, pid);

	TizenSyncAdapter *pSyncAdapter = tizen_sync_adapter_proxy_new_sync(connection,
			G_DBUS_PROXY_FLAGS_NONE,
			"org.tizen.sync",
			obj_path,
			NULL,
			&error);
	SYNC_LOGE_RET_RES(error == NULL && pSyncAdapter != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

	tizen_sync_adapter_call_init_complete_sync(pSyncAdapter, NULL, NULL);
	g_signal_connect(pSyncAdapter, "start-sync", G_CALLBACK(sync_adapter_on_start_sync), NULL);
	g_signal_connect(pSyncAdapter, "cancel-sync", G_CALLBACK(sync_adapter_on_stop_sync), NULL);

	g_sync_adapter->sync_adapter_obj = pSyncAdapter;
	g_sync_adapter->start_sync_cb = on_start_cb;
	g_sync_adapter->cancel_sync_cb = on_cancel_cb;

	LOG_LOGD("sync adapter callbacks are set %s", obj_path);

	return SYNC_ERROR_NONE;
}


int sync_adapter_unset_callbacks()
{
	SYNC_LOGE_RET_RES(g_sync_adapter != NULL, SYNC_ERROR_SYSTEM, "sync_adapter_init should be called first");

	g_sync_adapter->start_sync_cb = NULL;
	g_sync_adapter->cancel_sync_cb = NULL;

	return SYNC_ERROR_NONE;
}
