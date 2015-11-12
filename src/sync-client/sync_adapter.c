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
#include <app_manager.h>
#include <pkgmgr-info.h>
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

#define SYNC_MANAGER_DBUS_SERVICE "org.tizen.sync"
#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_COMMON_DBUS_PATH "/org/tizen/sync/adapter"


typedef struct sync_adapter_s {
	TizenSyncAdapter *sync_adapter_obj;
	bool __syncRunning;
	sync_adapter_start_sync_cb start_sync_cb;
	sync_adapter_cancel_sync_cb cancel_sync_cb;
} sync_adapter_s;

static sync_adapter_s *g_sync_adapter = NULL;

extern int read_proc(const char *path, char *buf, int size);
extern char *proc_get_cmdline_self();

gboolean
__sync_adapter_on_start_sync(TizenSyncAdapter *pObject,
											gint accountId,
											const gchar *pSyncJobName,
											gboolean is_data_sync,
											GVariant *pSyncJobUserData)
{
	SYNC_LOGE_RET_RES(pObject != NULL && pSyncJobName != NULL, true, "sync adapter object is null");
	LOG_LOGD("Received start sync request in sync adapter: params account[%d] jobname [%s] Data sync [%s]", accountId, pSyncJobName, is_data_sync ? "YES" : "NO");

	if (g_sync_adapter->__syncRunning) {
		LOG_LOGD("Sync already running");

		tizen_sync_adapter_call_send_result_sync(pObject, (int)SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS, pSyncJobName, NULL, NULL);

		return true;
	}

	g_sync_adapter->__syncRunning = true;

	int ret = SYNC_STATUS_SUCCESS;
	account_h account = NULL;
	if (accountId != -1) {
		account_create(&account);
		account_query_account_by_account_id(accountId, &account);
	}
	bundle *sync_job_user_data = umarshal_bundle(pSyncJobUserData);

	bool is_sync_success;
	if (is_data_sync)
		is_sync_success = g_sync_adapter->start_sync_cb(account, NULL, pSyncJobName, sync_job_user_data);
	else
		is_sync_success = g_sync_adapter->start_sync_cb(account, pSyncJobName, NULL, sync_job_user_data);

	if (!is_sync_success)
		ret = SYNC_STATUS_FAILURE;

	tizen_sync_adapter_call_send_result_sync(pObject, ret, pSyncJobName, NULL, NULL);
	g_sync_adapter->__syncRunning = false;
	bundle_free(sync_job_user_data);
	LOG_LOGD("Sync completed");

	if (ret == SYNC_STATUS_FAILURE)
		return false;

	return true;
}


gboolean
__sync_adapter_on_stop_sync(
		TizenSyncAdapter *pObject,
		gint accountId,
		const gchar *pSyncJobName,
		gboolean is_data_sync,
		GVariant *pSyncJobUserData)
{
	LOG_LOGD("handle stop in adapter");

	SYNC_LOGE_RET_RES(pObject != NULL, true, "sync adapter object is null");
	LOG_LOGD("Received stop sync request in sync adapter: params account[%d] jobname [%s] Data sync [%s]", accountId, pSyncJobName, is_data_sync ? "YES" : "NO");

	account_h account = NULL;
	if (accountId != -1) {
		account_create(&account);
		account_query_account_by_account_id(accountId, &account);
	}
	bundle *sync_job_user_data = umarshal_bundle(pSyncJobUserData);

	if (is_data_sync)
		g_sync_adapter->cancel_sync_cb(account, NULL, pSyncJobName, sync_job_user_data);
	else
		g_sync_adapter->cancel_sync_cb(account, pSyncJobName, NULL, sync_job_user_data);

	return true;
}


/* flag == true => register */
/* flag == false => de-register */
int __register_sync_adapter(bool flag)
{
	bool ret = true;

	GError *error = NULL;
	GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
	SYNC_LOGE_RET_RES(connection != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

	TizenSyncManager *ipcObj = tizen_sync_manager_proxy_new_sync(connection,
																	G_DBUS_PROXY_FLAGS_NONE,
																	SYNC_MANAGER_DBUS_SERVICE,
																	SYNC_MANAGER_DBUS_PATH,
																	NULL,
																	&error);
	SYNC_LOGE_RET_RES(error == NULL && ipcObj != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

	char *command_line = proc_get_cmdline_self();
	if (flag)
		ret = tizen_sync_manager_call_add_sync_adapter_sync(ipcObj, command_line, NULL, &error);
	else
		tizen_sync_manager_call_remove_sync_adapter_sync(ipcObj, command_line, NULL, &error);

	free(command_line);

	SYNC_LOGE_RET_RES(ret, SYNC_ERROR_QUOTA_EXCEEDED, "Register sync adapter failed %s", error->message);
	SYNC_LOGE_RET_RES(error == NULL, SYNC_ERROR_IO_ERROR, "Register sync adapter failed %s", error->message);

	return SYNC_ERROR_NONE;
}


int sync_adapter_set_callbacks(sync_adapter_start_sync_cb on_start_cb, sync_adapter_cancel_sync_cb on_cancel_cb)
{
	SYNC_LOGE_RET_RES(on_start_cb != NULL && on_cancel_cb != NULL, SYNC_ERROR_INVALID_PARAMETER, "Callback parameters must be passed");

	if (!g_sync_adapter) {
		int ret = __register_sync_adapter(true);
		if (ret == SYNC_ERROR_NONE) {
			pid_t pid = getpid();
			GError *error = NULL;
			GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
			SYNC_LOGE_RET_RES(connection != NULL, SYNC_ERROR_SYSTEM, "System error occured %s", error->message);

			char obj_path[50];
			snprintf(obj_path, 50, "%s/%d", SYNC_ADAPTER_COMMON_DBUS_PATH, pid);

			TizenSyncAdapter *pSyncAdapter = tizen_sync_adapter_proxy_new_sync(connection,
																			G_DBUS_PROXY_FLAGS_NONE,
																			SYNC_MANAGER_DBUS_SERVICE,
																			obj_path,
																			NULL,
																			&error);
			SYNC_LOGE_RET_RES(error == NULL && pSyncAdapter != NULL, SYNC_ERROR_IO_ERROR, "tizen_sync_manager_proxy_new_sync failed %s", error->message);

			g_signal_connect(pSyncAdapter, "start-sync", G_CALLBACK(__sync_adapter_on_start_sync), NULL);
			g_signal_connect(pSyncAdapter, "cancel-sync", G_CALLBACK(__sync_adapter_on_stop_sync), NULL);

			g_sync_adapter = (sync_adapter_s *) malloc(sizeof(struct sync_adapter_s));
			SYNC_LOGE_RET_RES(g_sync_adapter != NULL, SYNC_ERROR_OUT_OF_MEMORY, "Out of memory");

			g_sync_adapter->sync_adapter_obj = NULL;
			g_sync_adapter->__syncRunning = false;
			g_sync_adapter->sync_adapter_obj = pSyncAdapter;
		} else
			return ret;
	}

	g_sync_adapter->start_sync_cb = on_start_cb;
	g_sync_adapter->cancel_sync_cb = on_cancel_cb;

	return SYNC_ERROR_NONE;
}


int sync_adapter_unset_callbacks()
{
	SYNC_LOGE_RET_RES(g_sync_adapter != NULL, SYNC_ERROR_SYSTEM, "sync_adapter_set_callbacks should be called first");

	__register_sync_adapter(false);
	g_sync_adapter->start_sync_cb = NULL;
	g_sync_adapter->cancel_sync_cb = NULL;

	LOG_LOGD("sync_adapter_destroy");
	if (g_sync_adapter) {
		free(g_sync_adapter);
		g_sync_adapter = NULL;
	}

	return SYNC_ERROR_NONE;
}
