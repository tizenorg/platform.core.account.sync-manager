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
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <app_manager.h>
#include <bundle_internal.h>
#include "sync-ipc-marshal.h"
#include "sync-manager-stub.h"
#include "sync-adapter-stub.h"
#include "sync_manager.h"
#include "sync-log.h"
#include "sync-error.h"

#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_DBUS_PATH "/org/tizen/sync/adapter"
#define SYNC_ERROR_PREFIX "org.tizen.sync.Error"


typedef struct sync_manager_s {
	TizenSyncManager *ipcObj;
	sync_manager_sync_job_cb sync_job_cb;
	char *appid;
} sync_manager_s;

static sync_manager_s *g_sync_manager;

int read_proc(const char *path, char *buf, int size)
{
	int fd;
	int ret;

	if (buf == NULL || path == NULL)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		LOG_LOGD("fd = %d", fd);

		return -1;
	}

	ret = read(fd, buf, size - 1);
	if (ret <= 0) {
		close(fd);

		return -1;
	} else
		buf[ret] = 0;

	close(fd);

	return ret;
}

char *proc_get_cmdline_self()
{
	char cmdline[1024];
	int ret;

	ret = read_proc("/proc/self/cmdline", cmdline, sizeof(cmdline));
	if (ret <= 0) {
		LOG_LOGD("Can not read /proc/self/cmdline");
		return NULL;
	}
	LOG_LOGD("sync client: cmdLine [%s]", cmdline);

	return strdup(cmdline);
}


GDBusErrorEntry _sync_errors[] = {
	{SYNC_ERROR_NONE, SYNC_ERROR_PREFIX".NoError"},
	{SYNC_ERROR_OUT_OF_MEMORY, SYNC_ERROR_PREFIX".OutOfMemory"},
	{SYNC_ERROR_IO_ERROR, SYNC_ERROR_PREFIX".IOError"},
	{SYNC_ERROR_PERMISSION_DENIED, SYNC_ERROR_PREFIX".PermissionDenied"},
	{SYNC_ERROR_ALREADY_IN_PROGRESS, SYNC_ERROR_PREFIX".AlreadyInProgress"},
	{SYNC_ERROR_INVALID_OPERATION, SYNC_ERROR_PREFIX".InvalidOperation"},
	{SYNC_ERROR_INVALID_PARAMETER, SYNC_ERROR_PREFIX".InvalidParameter"},
	{SYNC_ERROR_QUOTA_EXCEEDED, SYNC_ERROR_PREFIX".QuotaExceeded"},
	{SYNC_ERROR_UNKNOWN, SYNC_ERROR_PREFIX".Unknown"},
	{SYNC_ERROR_SYSTEM, SYNC_ERROR_PREFIX".System"},
	{SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND, SYNC_ERROR_PREFIX".SyncAdapterIsNotFound"},
};


static int _sync_get_error_code(bool is_success, GError *error)
{
	if (!is_success) {
		LOG_LOGD("Received error Domain[%d] Message[%s] Code[%d]", error->domain, error->message, error->code);

		if (g_dbus_error_is_remote_error(error)) {
			gchar *remote_error = g_dbus_error_get_remote_error(error);
			if (remote_error) {
				LOG_LOGD("Remote error[%s]", remote_error);

				int error_enum_count = G_N_ELEMENTS(_sync_errors);
				int i = 0;
				for (i = 0; i < error_enum_count; i++) {
					if (g_strcmp0(_sync_errors[i].dbus_error_name, remote_error) == 0) {
						LOG_LOGD("Remote error code matched[%d]", _sync_errors[i].error_code);
						return _sync_errors[i].error_code;
					}
				}
			}
		}
		/*All undocumented errors mapped to SYNC_ERROR_UNKNOWN*/
		return SYNC_ERROR_UNKNOWN;
	}
	return SYNC_ERROR_NONE;
}


static int initialize_connection()
{
	SYNC_LOGE_RET_RES(g_sync_manager == NULL, SYNC_ERROR_NONE, "sync manager already connected");

	pid_t pid = getpid();
	char *appId = NULL;

	GDBusConnection *connection = NULL;
	GError *error = NULL;
	connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

	TizenSyncManager *ipcObj = tizen_sync_manager_proxy_new_sync(connection,
			G_DBUS_PROXY_FLAGS_NONE,
			"org.tizen.sync",
			"/org/tizen/sync/manager",
			NULL,
			&error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_sync_manager = NULL;

		return SYNC_ERROR_IO_ERROR;
	}

	if (ipcObj == NULL) {
		g_sync_manager = NULL;

		return SYNC_ERROR_SYSTEM;
	}

	g_sync_manager = (sync_manager_s *) malloc(sizeof(struct sync_manager_s));
	if (g_sync_manager == NULL)
		return SYNC_ERROR_OUT_OF_MEMORY;

	int ret = app_manager_get_app_id(pid, &appId);
	if (ret != APP_MANAGER_ERROR_NONE)
		appId = proc_get_cmdline_self();
	g_sync_manager->ipcObj = ipcObj;
	g_sync_manager->appid = appId;

	return SYNC_ERROR_NONE;
}


int get_interval(sync_period_e period)
{
	int frequency = 0;
	switch (period) {
	case SYNC_PERIOD_INTERVAL_30MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_30MIN");
		frequency = 30;
		break;
	}
	case SYNC_PERIOD_INTERVAL_1H:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_1H");
		frequency = 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_2H:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_2H");
		frequency = 2 * 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_3H:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_3H");
		frequency = 3 * 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_6H:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_6H");
		frequency = 6 * 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_12H:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_12H");
		frequency = 12 * 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_1DAY:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_1DAY");
		frequency = 24 * 60;
		break;
	}
	case SYNC_PERIOD_INTERVAL_MAX:
	default:
	{
		LOG_LOGD("Error");
		break;
	}
	}

	return frequency * 60;		/*return in seconds*/
}


int sync_manager_on_demand_sync_job(account_h account, const char *sync_job_name, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id)
{
	if (!g_sync_manager) {
		LOG_LOGD("Sync manager is not initialized yet");
		initialize_connection();
	}

	SYNC_LOGE_RET_RES(sync_job_name != NULL, SYNC_ERROR_INVALID_PARAMETER, "sync_job_name is NULL");
	SYNC_LOGE_RET_RES(sync_option >= SYNC_OPTION_NONE && sync_option <= (SYNC_OPTION_EXPEDITED | SYNC_OPTION_NO_RETRY), SYNC_ERROR_INVALID_PARAMETER, "sync_option is invalid %d", sync_option);
	SYNC_LOGE_RET_RES(sync_job_id != NULL, SYNC_ERROR_INVALID_PARAMETER, "sync_job_id is NULL");

	LOG_LOGC("sync client: %s requesting one time sync", g_sync_manager->appid);

	int ret = ACCOUNT_ERROR_NONE;
	int id = -1;
	if (account) {
		ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");
			return SYNC_ERROR_SYSTEM;
		}
		LOG_LOGC("appid [%s] accid [%d] sync_job_name [%s] ", g_sync_manager->appid, id, sync_job_name);
	} else
		LOG_LOGC("appid [%s] sync_job_name [%s] ", g_sync_manager->appid, sync_job_name);

	GError *error = NULL;
	GVariant *user_data = marshal_bundle(sync_job_user_data);

	bool is_success = tizen_sync_manager_call_add_on_demand_sync_job_sync(g_sync_manager->ipcObj, id, sync_job_name, sync_option, user_data, sync_job_id, NULL, &error);
	if (!is_success || error) {
		int error_code = _sync_get_error_code(is_success, error);
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_clear_error(&error);

		return error_code;
	}
	if (*sync_job_id == -1)
		return SYNC_ERROR_QUOTA_EXCEEDED;


	return SYNC_ERROR_NONE;
}


int sync_manager_add_periodic_sync_job(account_h account, const char *sync_job_name, sync_period_e sync_period, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id)
{
	if (!g_sync_manager) {
		LOG_LOGD("Sync manager is not initialized yet");
		initialize_connection();
	}

	SYNC_LOGE_RET_RES(sync_job_name != NULL, SYNC_ERROR_INVALID_PARAMETER, "sync_job_name is NULL");
	SYNC_LOGE_RET_RES((sync_period >= SYNC_PERIOD_INTERVAL_30MIN && sync_period < SYNC_PERIOD_INTERVAL_MAX), SYNC_ERROR_INVALID_PARAMETER, "Time interval not supported %d", sync_period);
	SYNC_LOGE_RET_RES(sync_option >= SYNC_OPTION_NONE && sync_option <= (SYNC_OPTION_EXPEDITED | SYNC_OPTION_NO_RETRY), SYNC_ERROR_INVALID_PARAMETER, "sync_option is invalid %d", sync_option);
	SYNC_LOGE_RET_RES(sync_job_id != NULL, SYNC_ERROR_INVALID_PARAMETER, "sync_job_id is NULL");

	LOG_LOGC("sync client: %s requesting periodic sync", g_sync_manager->appid);

	int ret = ACCOUNT_ERROR_NONE;
	int id = -1;
	if (account) {
		ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");
			return SYNC_ERROR_SYSTEM;
		}
		LOG_LOGC("appid [%s] accid [%d] sync_job_name [%s] ", g_sync_manager->appid, id, sync_job_name);
	} else
		LOG_LOGC("appid [%s] sync_job_name [%s] ", g_sync_manager->appid, sync_job_name);

	int sync_interval = get_interval(sync_period);

	GError *error = NULL;
	GVariant *user_data = marshal_bundle(sync_job_user_data);

	bool is_success = tizen_sync_manager_call_add_periodic_sync_job_sync(g_sync_manager->ipcObj, id, sync_job_name, sync_interval, sync_option, user_data, sync_job_id, NULL, &error);
	if (!is_success || error) {
		int error_code = _sync_get_error_code(is_success, error);
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_clear_error(&error);

		return error_code;
	}

	return SYNC_ERROR_NONE;
}


int sync_manager_add_data_change_sync_job(account_h account, const char *sync_capability, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id)
{
	if (!g_sync_manager) {
		LOG_LOGD("Sync manager is not initialized yet");
		initialize_connection();
	}

	if (sync_capability != NULL) {
		if (!(strcmp(sync_capability, "http://tizen.org/sync/capability/calendar")) ||
			!(strcmp(sync_capability, "http://tizen.org/sync/capability/contact")) ||
			!(strcmp(sync_capability, "http://tizen.org/sync/capability/image")) ||
			!(strcmp(sync_capability, "http://tizen.org/sync/capability/video")) ||
			!(strcmp(sync_capability, "http://tizen.org/sync/capability/sound")) ||
			!(strcmp(sync_capability, "http://tizen.org/sync/capability/music"))) {
			LOG_LOGC("sync client: capability [%s] ", sync_capability);
		} else {
			LOG_LOGD("sync client: invalid capability");
			return SYNC_ERROR_INVALID_PARAMETER;
		}
	} else {
		LOG_LOGD("sync client: sync_capability is NULL");
		return SYNC_ERROR_INVALID_PARAMETER;
	}

	SYNC_LOGE_RET_RES(sync_option >= SYNC_OPTION_NONE && sync_option <= (SYNC_OPTION_EXPEDITED | SYNC_OPTION_NO_RETRY), SYNC_ERROR_INVALID_PARAMETER, "sync_option is invalid %d", sync_option);
	SYNC_LOGE_RET_RES(sync_job_id != NULL, SYNC_ERROR_INVALID_PARAMETER, "sync_job_id is NULL");

	LOG_LOGC("sync client: %s requesting data change callback", g_sync_manager->appid);

	int ret = ACCOUNT_ERROR_NONE;
	int id = -1;
	if (account) {
		ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");
			return SYNC_ERROR_SYSTEM;
		}
		LOG_LOGC("appid [%s] accid [%d] capability [%s] ", g_sync_manager->appid, id, sync_capability);
	} else
		LOG_LOGC("appid [%s] capability [%s] ", g_sync_manager->appid, sync_capability);

	GError *error = NULL;
	GVariant *user_data = marshal_bundle(sync_job_user_data);

	bool is_success = tizen_sync_manager_call_add_data_change_sync_job_sync(g_sync_manager->ipcObj, id, sync_capability, sync_option, user_data, sync_job_id, NULL, &error);
	if (!is_success || error) {
		int error_code = _sync_get_error_code(is_success, error);
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_clear_error(&error);

		return error_code;
	}

	return SYNC_ERROR_NONE;
}


int sync_manager_remove_sync_job(int sync_job_id)
{
	if (!g_sync_manager) {
		LOG_LOGD("Sync manager is not initialized yet");
		initialize_connection();
	}
	SYNC_LOGE_RET_RES(g_sync_manager->ipcObj != NULL, SYNC_ERROR_SYSTEM, "sync manager is not connected");
	SYNC_LOGE_RET_RES(sync_job_id >= 1 && sync_job_id <= 100, SYNC_ERROR_INVALID_PARAMETER, "sync_job_id is inappropriate value");

	LOG_LOGC("sync client: %s removing sync job with sync_job_id [%d] ", g_sync_manager->appid, sync_job_id);

	GError *error = NULL;
	bool is_success = tizen_sync_manager_call_remove_sync_job_sync(g_sync_manager->ipcObj, sync_job_id, NULL, &error);

	if (!is_success || error) {
		int error_code = _sync_get_error_code(is_success, error);
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_clear_error(&error);

		return error_code;
	}
	return SYNC_ERROR_NONE;
}


int sync_manager_foreach_sync_job(sync_manager_sync_job_cb sync_job_cb, void *user_data)
{
	if (!sync_job_cb) {
		LOG_LOGD("sync client: sync_job_cb is NULL");
		return SYNC_ERROR_INVALID_PARAMETER;
	}

	if (!g_sync_manager) {
		LOG_LOGD("Sync manager is not initialized yet");
		initialize_connection();
	}

	g_sync_manager->sync_job_cb = sync_job_cb;

	GError *error = NULL;
	GVariant *sync_job_list_variant = NULL;
	gboolean is_success = tizen_sync_manager_call_get_all_sync_jobs_sync(g_sync_manager->ipcObj, &sync_job_list_variant, NULL, &error);

	if (!is_success || error) {
		int error_code = _sync_get_error_code(is_success, error);
		LOG_LOGC("sync client: gdbus error [%s]", error->message);
		g_clear_error(&error);

		return error_code;
	} else {
		umarshal_sync_job_list(sync_job_list_variant, sync_job_cb, user_data);
	}

	return SYNC_ERROR_NONE;
}


int _sync_manager_enable_sync()
{
	GError *error = NULL;
	tizen_sync_manager_call_set_sync_status_sync(g_sync_manager->ipcObj, true, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}


int _sync_manager_disable_sync()
{
	GError *error = NULL;
	tizen_sync_manager_call_set_sync_status_sync(g_sync_manager->ipcObj, false, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}
