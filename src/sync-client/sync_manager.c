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


typedef struct sync_manager_s {
	TizenSyncManager *ipcObj;
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
	if (ret <= 0)
		return NULL;

	return strdup(cmdline);
}


int sync_manager_connect(void)
{
	SYNC_LOGE_RET_RES(g_sync_manager == NULL, SYNC_ERROR_NONE, "sync manager already connected");

	pid_t pid = getpid();
	char *appId = NULL;

	int ret = app_manager_get_app_id(pid, &appId);
	if (ret != APP_MANAGER_ERROR_NONE)
		appId = proc_get_cmdline_self();

	GDBusConnection *connection = NULL;
	GError *error = NULL;
	connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

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

	g_sync_manager->ipcObj = ipcObj;
	g_sync_manager->appid = appId;

	return SYNC_ERROR_NONE;
}


void sync_manager_disconnect(void)
{
	if (g_sync_manager) {
		if (g_sync_manager->appid)
			free(g_sync_manager->appid);

		free(g_sync_manager);
		g_sync_manager = NULL;
	}
}


int sync_manager_add_sync_job(account_h account, const char *capability, bundle *extra)
{
	SYNC_LOGE_RET_RES(g_sync_manager != NULL, SYNC_ERROR_SYSTEM, "sync_manager_connected should be called first");
	SYNC_LOGE_RET_RES(g_sync_manager->ipcObj != NULL, SYNC_ERROR_SYSTEM, "sync manager is not connected");

	LOG_LOGC("sync client: %s requesting one time Sync", g_sync_manager->appid);

	bool is_account_less_sync = false;
	if (account == NULL && capability == NULL) {
		is_account_less_sync = true;
		LOG_LOGC("sync client: initiating account less sync");
	}

	if (capability != NULL && strstr(capability, "http://tizen.org/account/capability/") == NULL) {
		LOG_LOGC("sync client: invalid capability");

		return SYNC_ERROR_INVALID_PARAMETER;
	}

	GVariant *ext_gvariant = NULL;
	if (extra != NULL) {
		ext_gvariant = marshal_bundle(extra);
		if (ext_gvariant == NULL) {
			LOG_LOGC("sync client: marshalling failed");

			return SYNC_ERROR_SYSTEM;
		}
	}

	int id = -1;
	if (!is_account_less_sync) {
		int ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");

			return SYNC_ERROR_INVALID_PARAMETER;
		}
		LOG_LOGC("sync client: account_get_account_id = %d", id);
	}

	GError *error = NULL;
	tizen_sync_manager_call_add_sync_job_sync(g_sync_manager->ipcObj, is_account_less_sync, id, ext_gvariant, capability, g_sync_manager->appid, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}


int sync_manager_remove_sync_job(account_h account, const char *capability)
{
	SYNC_LOGE_RET_RES(g_sync_manager != NULL, SYNC_ERROR_SYSTEM, "sync_manager_connected should be called first");
	SYNC_LOGE_RET_RES(g_sync_manager->ipcObj != NULL, SYNC_ERROR_SYSTEM, "sync manager is not connected");

	LOG_LOGC("sync client: %s remove sync job", g_sync_manager->appid);

	bool is_account_less_sync = false;
	if (account == NULL && capability == NULL) {
		is_account_less_sync = true;
		LOG_LOGC("sync client: initiating account less request");
	}

	if (capability != NULL && strstr(capability, "http://tizen.org/account/capability/") == NULL) {
		LOG_LOGC("sync client: invalid capability");

		return SYNC_ERROR_INVALID_PARAMETER;
	}

	int id = -1;
	if (!is_account_less_sync) {
		int ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");

			return SYNC_ERROR_INVALID_PARAMETER;
		}
		LOG_LOGC("sync client: account_get_account_id = %d", id);
	}

	GError *error = NULL;
	tizen_sync_manager_call_remove_sync_job_sync(g_sync_manager->ipcObj, is_account_less_sync, id, capability, g_sync_manager->appid, NULL, &error);

	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}


int get_interval(sync_period_e period)
{
	int frequency = 0;
	switch (period) {
	case SYNC_PERIOD_INTERVAL_5MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_5MIN");
		frequency = 5;
		break;
	}
	case SYNC_PERIOD_INTERVAL_10MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_10MIN");
		frequency = 10;
		break;
	}
	case SYNC_PERIOD_INTERVAL_15MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_15MIN");
		frequency = 15;
		break;
	}
	case SYNC_PERIOD_INTERVAL_20MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_20MIN");
		frequency = 20;
		break;
	}
	case SYNC_PERIOD_INTERVAL_30MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_30MIN");
		frequency = 30;
		break;
	}
	case SYNC_PERIOD_INTERVAL_45MIN:
	{
		LOG_LOGD("SYNC_PERIOD_INTERVAL_45MIN");
		frequency = 45;
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


int sync_manager_add_periodic_sync_job(account_h account, const char *capability, bundle *extra, sync_period_e sync_period)
{
	SYNC_LOGE_RET_RES(g_sync_manager != NULL, SYNC_ERROR_SYSTEM, "sync_manager_connected should be called first");
	SYNC_LOGE_RET_RES(g_sync_manager->ipcObj != NULL, SYNC_ERROR_SYSTEM, "sync manager is not connected");
	SYNC_LOGE_RET_RES((sync_period >= SYNC_PERIOD_INTERVAL_5MIN && sync_period < SYNC_PERIOD_INTERVAL_MAX), SYNC_ERROR_INVALID_PARAMETER, "Time interval not supported %d", sync_period);

	bool is_account_less_sync = false;
	if (account == NULL && capability == NULL)
		is_account_less_sync = true;

	if (capability != NULL && strstr(capability, "http://tizen.org/account/capability/") == NULL) {
		LOG_LOGC("sync client: invalid capability %s", capability);
		return SYNC_ERROR_INVALID_PARAMETER;
	}
	int id = -1;
	if (!is_account_less_sync) {
		int ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");

			return SYNC_ERROR_INVALID_PARAMETER;
		}
		LOG_LOGC("appid [%s] accid [%d] capability [%s] ", g_sync_manager->appid, id, capability);
	} else
		LOG_LOGC("appid [%s] ", g_sync_manager->appid);

	int sync_interval = get_interval(sync_period);

	bool doNotRetry = false;
	bool expedited = false;

	const char *pVal = bundle_get_val(extra, SYNC_OPTION_NO_RETRY);
	if (pVal != NULL && strcmp(pVal, "true") == 0) {
		doNotRetry = true;
		pVal = NULL;
	}

	pVal = bundle_get_val(extra, SYNC_OPTION_EXPEDITED);
	if (pVal != NULL && strcmp(pVal, "true") == 0) {
		expedited = true;
		pVal = NULL;
	}

	if (doNotRetry || expedited)
		return SYNC_ERROR_INVALID_PARAMETER;

	GError *error = NULL;
	GVariant *ext = marshal_bundle(extra);
	if (ext == NULL)
		return SYNC_ERROR_UNKNOWN;

	tizen_sync_manager_call_add_periodic_sync_job_sync(g_sync_manager->ipcObj, is_account_less_sync, id, ext, capability, sync_interval, g_sync_manager->appid, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}


int sync_manager_remove_periodic_sync_job(account_h account, const char *capability)
{
	SYNC_LOGE_RET_RES(g_sync_manager != NULL, SYNC_ERROR_SYSTEM, "sync_manager_connected should be called first");
	SYNC_LOGE_RET_RES(g_sync_manager->ipcObj != NULL, SYNC_ERROR_SYSTEM, "sync manager is not connected");

	LOG_LOGC("sync client: %s removing periodic sync job", g_sync_manager->appid);

	bool is_account_less_sync = false;
	if (account == NULL && capability == NULL) {
		is_account_less_sync = true;
		LOG_LOGC("sync client: initiating account less request");
	}

	if (capability != NULL && strstr(capability, "http://tizen.org/account/capability/") == NULL) {
		LOG_LOGC("sync client: invalid capability");

		return SYNC_ERROR_INVALID_PARAMETER;
	}

	int id = -1;
	if (!is_account_less_sync) {
		int ret = account_get_account_id(account, &id);
		if (ret != 0) {
			LOG_LOGC("sync client: account_get_account_id failure");

			return SYNC_ERROR_INVALID_PARAMETER;
		}
		LOG_LOGC("sync client: account_get_account_id = %d", id);
	}

	GError *error = NULL;
	tizen_sync_manager_call_remove_periodic_sync_job_sync(g_sync_manager->ipcObj, is_account_less_sync, id, capability, g_sync_manager->appid, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}


int _sync_manager_enable_sync()
{
	if (!g_sync_manager)
		sync_manager_connect();

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
	if (!g_sync_manager)
		sync_manager_connect();

	GError *error = NULL;
	tizen_sync_manager_call_set_sync_status_sync(g_sync_manager->ipcObj, false, NULL, &error);
	if (error != NULL) {
		LOG_LOGC("sync client: gdbus error [%s]", error->message);

		return SYNC_ERROR_IO_ERROR;
	}

	return SYNC_ERROR_NONE;
}
