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
 * @file	SyncManager_SyncManager.cpp
 * @brief	This is the implementation file for the SyncService class.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <glib.h>
#include <app.h>
#include <aul.h>
#include <app_manager.h>
#include <pkgmgr-info.h>
//#include <security-server.h>
#include "sync-error.h"
#include "SyncManager_SyncManager.h"
#include "sync-manager-stub.h"
#include "sync-ipc-marshal.h"
#include "sync-adapter-stub.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncJobsInfo.h"
#include "SyncManager_SyncAdapterAggregator.h"
#include "SyncManager_SyncJobsAggregator.h"

static GDBusConnection* gdbusConnection = NULL;

/*namespace _SyncManager
{*/

#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_DBUS_PATH "/org/tizen/sync/adapter/"
#define SYNC_ERROR_DOMAIN "sync-manager"
#define SYNC_ERROR_PREFIX "org.tizen.sync.Error"
#define ALARM_SET_LABEL  "alarm-server::alarm"
#define CALENDAR_READ_LABEL  "calendar-service::svc"
#define CONTACT_READ_LABEL  "contacts-service::svc"
#define READ_PERM "r"
#define WRITE_PERM "w"

GDBusObjectManagerServer* pServerManager = NULL;


static TizenSyncManager* sync_ipc_obj = NULL;
GHashTable* g_hash_table = NULL;
string sa_app_id;

using namespace std;

void convert_to_path(char app_id[])
{
	int i = 0;
	while (app_id[i] != '\0')
	{
		if (app_id[i] == '.' || app_id[i] == '-')
		{
			app_id[i] = '_';
		}
		i++;
	}
}

int
SyncService::StartService()
{
	__pSyncMangerIntacnce = SyncManager::GetInstance();
	if (__pSyncMangerIntacnce == NULL)
	{
		LOG_LOGD("Failed to initialize sync manager");
		return -1;
	}

	return 0;
}

TizenSyncAdapter* adapter;

GDBusErrorEntry _sync_errors[] =
{
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

static GQuark
_sync_error_quark (void)
{
	static volatile gsize quark_volatile = 0;

	g_dbus_error_register_error_domain (SYNC_ERROR_DOMAIN,
										&quark_volatile,
										_sync_errors,
										G_N_ELEMENTS (_sync_errors));

	return (GQuark) quark_volatile;
}

/*
static int
_check_privilege_by_pid(const char *label, const char *access_perm, bool check_root, int pid) {
	guchar *cookie = NULL;
	gsize size = 0;
	int retval = 0;
	char buf[128] = {0,};
	FILE *fp = NULL;
	char title[128] = {0,};
	int uid = -1;

	if (check_root) {
		// Gets the userID from /proc/pid/status to check if the process is the root or not.
		snprintf(buf, sizeof(buf), "/proc/%d/status", pid);
		fp = fopen(buf, "r");
		if(fp) {
			while (fgets(buf, sizeof(buf), fp) != NULL) {
				if(strncmp(buf, "Uid:", 4) == 0) {
					sscanf(buf, "%s %d", title, &uid);
					break;
				}
			}
			fclose(fp);
		}
	}

	if (uid != 0) {	// Checks the privilege only when the process is not the root
		retval = security_server_check_privilege_by_pid(pid, label, access_perm);

		if (retval < 0) {
			if (retval == SECURITY_SERVER_API_ERROR_ACCESS_DENIED) {
				LOG_ERRORD("permission denied, app don't has \"%s %s\" smack rule.", label, access_perm);
			} else {
				LOG_ERRORD("Error has occurred in security_server_check_privilege_by_cookie() %d", retval);
			}
			return SYNC_ERROR_PERMISSION_DENIED;
		}
	}

	LOG_LOGD("The process(%d) was authenticated successfully.", pid);
	return SYNC_ERROR_NONE;
}
*/

int
SyncService::TriggerStartSync(const char* appId, int accountId, const char* syncJobName, bool isDataSync, bundle* pExtras)
{
	LOG_LOGD("appId [%s] jobname [%s]", appId, syncJobName);

	app_control_h app_control;
	int ret = SYNC_ERROR_NONE;

	int isRunning = aul_app_is_running(appId);
	if (isRunning == 0)
	{
		LOG_LOGD("app is not running, launch the app and wait for signal");
		ret = app_control_create(&app_control);
		SYNC_LOGE_RET_RES(ret == APP_CONTROL_ERROR_NONE, SYNC_ERROR_SYSTEM,"app control create failed %d", ret);

		ret = app_control_set_app_id(app_control, appId);
		if (ret != APP_CONTROL_ERROR_NONE)
		{
			LOG_LOGD("app control error %d", ret);
			app_control_destroy(app_control);
			return SYNC_ERROR_SYSTEM;
		}

		sa_app_id.clear();
		ret = app_control_send_launch_request(app_control, NULL, NULL);
		SYNC_LOGE_RET_RES(ret == APP_CONTROL_ERROR_NONE, SYNC_ERROR_SYSTEM, "app control launch request failed %d", ret);
	}
	else
	{
		LOG_LOGD("app is already running");
	}

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter)
	{
		GVariant* pBundle = marshal_bundle(pExtras);
		tizen_sync_adapter_emit_start_sync(pSyncAdapter, accountId, syncJobName, isDataSync, pBundle);
	}
	else
	{
		LOG_LOGD("Sync adapter entry not found");
		return SYNC_ERROR_SYSTEM;
	}

	return SYNC_ERROR_NONE;

}


void
SyncService::TriggerStopSync(const char* appId, int accountId, const char* syncJobName, bool isDataSync, bundle* pExtras)
{
	LOG_LOGD("Trigger stop sync %s", appId);

	int id = -1;

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Failed to lookup syncadapter");
		return;
	}
	GVariant* pBundle = marshal_bundle(pExtras);
	tizen_sync_adapter_emit_cancel_sync(pSyncAdapter, accountId, syncJobName, isDataSync, pBundle);
}


int
SyncService::RequestOnDemandSync(const char* pPackageId, const char* pSyncJobName, int accountId, bundle* pExtras, int syncOption, int* pSyncJobId)
{
	int ret = SYNC_ERROR_NONE;
	int syncJobId = -1;
	SyncJobsAggregator* pSyncJobsAggregator = __pSyncMangerIntacnce->GetSyncJobsAggregator();

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob)
	{
		SyncJob* pSyncJobEntry = dynamic_cast< SyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get sync job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption);
		LOG_LOGD("sync parameters are updated with new parameters", pSyncJobName);
	}
	else
	{
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncMangerIntacnce->AddOnDemandSync(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId);
	}

	if (ret == SYNC_ERROR_NONE)
	{
		*pSyncJobId = syncJobId;
	}

	return ret;
}


int
SyncService::RequestPeriodicSync(const char* pPackageId, const char* pSyncJobName, int accountId, bundle* pExtras, int syncOption, unsigned long pollFrequency, int* pSyncJobId)
{
	int ret = SYNC_ERROR_NONE;
	SyncJobsAggregator* pSyncJobsAggregator = __pSyncMangerIntacnce->GetSyncJobsAggregator();
	int syncJobId = -1;

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob)
	{
		PeriodicSyncJob* pSyncJobEntry = dynamic_cast< PeriodicSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get syn job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption, pollFrequency);
		LOG_LOGD("sync parameters are updated with new parameters", pSyncJobName);
	}
	else
	{
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncMangerIntacnce->AddPeriodicSyncJob(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId, pollFrequency);
	}

	if (ret == SYNC_ERROR_NONE)
	{
		*pSyncJobId = syncJobId;
	}
	return ret;
}


int
SyncService::RequestDataSync(const char* pPackageId, const char* pSyncJobName, int accountId, bundle* pExtras, int syncOption, const char* pCapability, int* pSyncJobId)
{
	int ret = SYNC_ERROR_NONE;
	SyncJobsAggregator* pSyncJobsAggregator = __pSyncMangerIntacnce->GetSyncJobsAggregator();
	int syncJobId = -1;

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob)
	{
		DataSyncJob* pSyncJobEntry = dynamic_cast< DataSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get syn job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption, pCapability);
		LOG_LOGD("sync parameters are updated with new parameters", pSyncJobName);
	}
	else
	{
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncMangerIntacnce->AddDataSyncJob(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId, pCapability);
	}

	if (ret == SYNC_ERROR_NONE)
	{
		*pSyncJobId = syncJobId;
	}
	return ret;
}


void
SyncService::HandleShutdown(void)
{
	LOG_LOGD("Shutdown Starts");

	SyncManager::GetInstance()->HandleShutdown();

	LOG_LOGD("Shutdown Ends");
}


static guint
get_caller_pid(GDBusMethodInvocation* pMethodInvocation)
{
	guint pid = -1;
	const char *name = NULL;
	name = g_dbus_method_invocation_get_sender(pMethodInvocation);
	if (name == NULL)
	{
		LOG_LOGD("GDbus error: Failed to get sender name");
		return -1;
	}

	GError *error = NULL;
	GDBusConnection* conn = g_dbus_method_invocation_get_connection(pMethodInvocation);
	GVariant *ret = g_dbus_connection_call_sync(conn,	"org.freedesktop.DBus",
														"/org/freedesktop/DBus",
														"org.freedesktop.DBus",
														"GetConnectionUnixProcessID",
														g_variant_new("(s)", name),
														NULL,
														G_DBUS_CALL_FLAGS_NONE,
														-1,
														NULL,
														&error);

	if (ret != NULL)
	{
		g_variant_get(ret, "(u)", &pid);
		g_variant_unref(ret);
	}

	return pid;
}


/*
* org.tizen.sync.adapter interface methods
*/
gboolean
sync_adapter_handle_send_result( TizenSyncAdapter* pObject, GDBusMethodInvocation* pInvocation,
																gint sync_result,
																const gchar* sync_job_name)
{
	LOG_LOGD("Received sync job result");
	guint pid = get_caller_pid(pInvocation);

	string pkgIdStr;
	int ret;
	char appId[1024] = {0,};
	if(aul_app_get_appid_bypid(pid, appId, sizeof(appId) - 1) == AUL_R_OK)
	{
		char pkgId[1024] = {0,};
		ret = aul_app_get_pkgname_bypid(pid, pkgId, (int)(sizeof(pkgId) - 1));
		if(ret != AUL_R_OK)
		{
			LOG_LOGD("Get pkg name by PID failed for [%s] ret = %d ", appId, ret);
		}
		else
		{
			pkgIdStr.append(pkgId);
		}
	}
	else
	{
		char commandLine[1024] = {0,};
		//ret = aul_app_get_cmdline_bypid(pid, commandLine, sizeof(commandLine) - 1);
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(commandLine);
	}

	if (!pkgIdStr.empty())
	{
		LOG_LOGD("Sync result received from [%s]: sync_job_name [%s] result [%d]", appId, sync_job_name, sync_result);

		SyncManager::GetInstance()->OnResultReceived((SyncStatus)sync_result, appId, pkgIdStr, sync_job_name);
	}
	else
		LOG_LOGD("Get package Id fail %d", ret);

	tizen_sync_adapter_complete_send_result(pObject, pInvocation);

	/// Syncadapter may kill self after sync.
	/// aul_terminate_pid(pid);

	return true;
}


gboolean
sync_adapter_handle_init_complete(TizenSyncAdapter* pObject, GDBusMethodInvocation* pInvocation)
{
	guint pid = get_caller_pid(pInvocation);
	char* pAppId = NULL;

	int ret = app_manager_get_app_id(pid, &pAppId);
	if(ret != APP_MANAGER_ERROR_NONE)
		LOG_LOGD("app_manager_get_app_id fail");

	sa_app_id.append(pAppId);

	LOG_LOGD("sync adapter client initialisation completed %s", pAppId);

	free(pAppId);
	//tizen_sync_adapter_complete_init_complete(pObject, pInvocation);

	return true;
}


/*
* org.tizen.sync.manager interface methods
*/
gboolean
sync_manager_add_on_demand_sync_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																gint accountId,
																const gchar* pSyncJobName,
																gint sync_option,
																GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received On-Demand Sync request");

	guint pid = get_caller_pid(pInvocation);
	string pkgId = SyncManager::GetInstance()->GetPkgIdByPID(pid);
	int ret = SYNC_ERROR_SYSTEM;
	int sync_job_id = 0;
	if(!pkgId.empty())
	{
		LOG_LOGD("Params acc[%d] name[%s] option[%d] package[%s]", accountId, pSyncJobName, sync_option, pkgId.c_str());
		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestOnDemandSync(pkgId.c_str(), pSyncJobName, accountId, pBundle, sync_option, &sync_job_id);
		bundle_free(pBundle);
	}
	else
	{
		LOG_LOGD("Failed to get package id");
	}

	if (ret != SYNC_ERROR_NONE)
	{
		GError* error = g_error_new (_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_on_demand_sync_job(pObject, pInvocation, sync_job_id);

	LOG_LOGD("End of On-Demand Sync request");

	return true;
}


gboolean
sync_manager_remove_sync_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, gint sync_job_id)
{
	LOG_LOGD("Request to remove sync job %d", sync_job_id);
	int ret = SYNC_ERROR_NONE;
	guint pid = get_caller_pid(pInvocation);

	string pkgId = SyncManager::GetInstance()->GetPkgIdByPID(pid);
	if(!pkgId.empty())
	{
		LOG_LOGD("package id [%s]", pkgId.c_str());
		SyncManager::GetInstance()->RemoveSyncJob(pkgId, sync_job_id);
	}

	tizen_sync_manager_complete_remove_sync_job(pObject, pInvocation);
	LOG_LOGD("sync service: remove sync job ends");
	return true;
}


gboolean
sync_manager_add_periodic_sync_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																gint accountId,
																const gchar* pSyncJobName,
																gint sync_interval,
																gint sync_option,
																GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received Period Sync request");

	int ret = SYNC_ERROR_NONE;
	guint pid = get_caller_pid(pInvocation);
/*
	ret = _check_privilege_by_pid(ALARM_SET_LABEL, WRITE_PERM, true, pid);
	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new (_sync_error_quark(), ret, "permission denied error");
		g_dbus_method_invocation_return_gerror (pInvocation, error);
		g_clear_error(&error);
		return true;
	}
*/
	int sync_job_id = 0;
	string pkgId = SyncManager::GetInstance()->GetPkgIdByPID(pid);
	if(!pkgId.empty())
	{
		LOG_LOGD("Params acc[%d] name[%s] option[%d] period[%d] package[%s]", accountId, pSyncJobName, sync_option, sync_interval, pkgId.c_str());
		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestPeriodicSync(pkgId.c_str(), pSyncJobName, accountId, pBundle, sync_option, sync_interval, &sync_job_id);
		bundle_free(pBundle);
	}

	if (ret != SYNC_ERROR_NONE)
	{
		GError* error = g_error_new (_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_periodic_sync_job(pObject, pInvocation, sync_job_id);

	LOG_LOGD("sync service: add periodic sync job ends");
	return true;
}


gboolean
sync_manager_add_data_change_sync_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																	gint accountId,
																	const gchar* pCapabilityArg,
																	gint sync_option,
																	GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received data change Sync request");
	int ret = SYNC_ERROR_NONE;
	guint pid = get_caller_pid(pInvocation);
/*
	const char *capability = (char *)pCapabilityArg;
	if (!strcmp(capability, "http://tizen.org/sync/capability/calendar") ||
		!strcmp(capability, "http://tizen.org/sync/capability/contact")) {
		if (!strcmp(capability, "http://tizen.org/sync/capability/calendar"))
			ret = _check_privilege_by_pid(CALENDAR_READ_LABEL, READ_PERM, true, pid);
		else
			ret = _check_privilege_by_pid(CONTACT_READ_LABEL, READ_PERM, true, pid);

		if (ret != SYNC_ERROR_NONE) {
			GError* error = g_error_new (_sync_error_quark(), ret, "permission denied error");
			g_dbus_method_invocation_return_gerror (pInvocation, error);
			g_clear_error(&error);
			return true;
		}
	}
*/
	int sync_job_id = 0;
	string pkgId = SyncManager::GetInstance()->GetPkgIdByPID(pid);
	if(!pkgId.empty())
	{
		LOG_LOGD("Params account [%d] job_name [%s] sync_option[%d] sync_job_id[%d] package [%s] ", accountId, pCapabilityArg, sync_option, sync_job_id, pkgId.c_str());

		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestDataSync(pkgId.c_str(), pCapabilityArg, accountId, pBundle, sync_option, pCapabilityArg, &sync_job_id);

		bundle_free(pBundle);
	}

	if (ret != SYNC_ERROR_NONE)
	{
		GError* error = g_error_new (_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_data_change_sync_job(pObject, pInvocation, sync_job_id);

	LOG_LOGD("sync service: add data sync job ends");
	return true;
}


static inline int __read_proc(const char *path, char *buf, int size)
{
	int fd;
	int ret;

	if (buf == NULL || path == NULL)
		return -1;

	fd = open(path, O_RDONLY);
	if (fd < 0)
	{
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


gboolean
sync_manager_add_sync_adapter(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine)
{
	LOG_LOGD("Received sync adapter registration request");
	guint pid = get_caller_pid(pInvocation);
	int ret = SYNC_ERROR_SYSTEM;
	string pkgIdStr;
	char appId[1024] = {0,};
	if(aul_app_get_appid_bypid(pid, appId, sizeof(appId) - 1) == AUL_R_OK)
	{
		char pkgId[1024] = {0,};
		int ret = aul_app_get_pkgname_bypid(pid, pkgId, (int)(sizeof(pkgId) - 1));
		if(ret != AUL_R_OK)
		{
			LOG_LOGD("Get pkgid by PID failed for [%s] ret = %d ", appId, ret);
		}
		else
		{
			pkgIdStr.append(pkgId);
		}
	}
	else
	{
		char commandLine[1024] = {0,};
		//ret = aul_app_get_cmdline_bypid(pid, commandLine, sizeof(commandLine) - 1);
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(commandLine);
	}

	if(!pkgIdStr.empty())
	{
		char object_path[50];
		snprintf(object_path, 50, "%s%d", SYNC_ADAPTER_DBUS_PATH, pid);

		GError *error = NULL;
		GDBusInterfaceSkeleton* interface = NULL;
		TizenSyncAdapter* syncAdapterObj= tizen_sync_adapter_skeleton_new();
		if (syncAdapterObj != NULL)
		{
			interface = G_DBUS_INTERFACE_SKELETON(syncAdapterObj);
			if (g_dbus_interface_skeleton_export(interface, gdbusConnection, object_path, &error))
			{
				g_signal_connect(syncAdapterObj, "handle-send-result", G_CALLBACK(sync_adapter_handle_send_result), NULL);
				g_signal_connect(syncAdapterObj, "handle-init-complete", G_CALLBACK(sync_adapter_handle_init_complete), NULL);

				SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();
				pAggregator->AddSyncAdapter(pkgIdStr.c_str(), appId);

				LOG_LOGD("inserting sync adapter ipc %s", appId);
				g_hash_table_insert(g_hash_table, strdup(appId), syncAdapterObj);
				ret = SYNC_ERROR_NONE;
			}
			else
			{
				LOG_LOGD("export failed %s", error->message);
			}
		}
		else
		{
			LOG_LOGD("sync adapter object creation failed");
		}
	}
	else
	{
		LOG_LOGD("Failed to get package ID");
	}

	if (ret != SYNC_ERROR_NONE)
	{
		GError* error = g_error_new (_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);

	LOG_LOGD("End");

	return true;
}


gboolean
sync_manager_remove_sync_adapter(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine)
{
	LOG_LOGD("Request to remove sync adapter");
	guint pid = get_caller_pid(pInvocation);
	string pkgIdStr;
	int ret;
	char appId[1024] = {0,};
	if(aul_app_get_appid_bypid(pid, appId, sizeof(appId) - 1) == AUL_R_OK)
	{
		char pkgId[1024] = {0,};
		ret = aul_app_get_pkgname_bypid(pid, pkgId, (int)(sizeof(pkgId) - 1));
		if(ret != AUL_R_OK)
		{
			LOG_LOGD("Get pkgid by PID failed for [%s] ret = %d ", appId, ret);
		}
		else
		{
			pkgIdStr.append(pkgId);
		}
	}
	else
	{
		char commandLine[1024] = {0,};
		//ret = aul_app_get_cmdline_bypid(pid, commandLine, sizeof(commandLine) - 1);
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(commandLine);
	}

	if(!pkgIdStr.empty())
	{
		SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();
		pAggregator->RemoveSyncAdapter(pkgIdStr.c_str());
		LOG_LOGD("Sync adapter removed for package [%s]", pkgIdStr.c_str());
	}

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Failed to lookup syncadapter gdbus object for [%s]", appId);
	}
	else
	{
		GDBusInterfaceSkeleton* interface = NULL;
		interface = G_DBUS_INTERFACE_SKELETON(pSyncAdapter);
		g_dbus_interface_skeleton_unexport(interface);
	}

	tizen_sync_manager_complete_remove_sync_adapter(pObject, pInvocation);

	LOG_LOGD("End");

	return true;
}


GVariant *
marshal_sync_job(ISyncJob* syncJob)
{
	SyncJob* pSyncJob = dynamic_cast< SyncJob* > (syncJob);
	GVariantBuilder builder;

	if(pSyncJob)
	{
		g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_ID, g_variant_new_int32 (pSyncJob->GetSyncJobId()));
		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_ACC_ID, g_variant_new_int32 (pSyncJob->__accountId));

		if (pSyncJob->GetSyncType() == SYNC_TYPE_DATA_CHANGE)
			g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_CAPABILITY, g_variant_new_string (pSyncJob->__syncJobName.c_str()));
		else
			g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_NAME, g_variant_new_string (pSyncJob->__syncJobName.c_str()));

		///LOG_LOGD("job name and id [%s] [%d]", pSyncJob->__syncJobName.c_str(), pSyncJob->GetSyncJobId());

		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_USER_DATA, marshal_bundle(pSyncJob->__pExtras));
	}

	return g_variant_builder_end (&builder);
}



gboolean
sync_manager_get_all_sync_jobs(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation)
{
	LOG_LOGD("Received request to get Sync job ids");

	int ret = SYNC_ERROR_SYSTEM;

	guint pid = get_caller_pid(pInvocation);
	string pkgId = SyncManager::GetInstance()->GetPkgIdByPID(pid);

	GVariant* outSyncJobList;
	GVariantBuilder builder;

	if(!pkgId.empty())
	{
		SyncJobsAggregator* pSyncJobsAggregator = SyncManager::GetInstance()->GetSyncJobsAggregator();
		SyncJobsInfo* pPackageSyncJobs = pSyncJobsAggregator->GetSyncJobsInfo(pkgId.c_str());

		if (pPackageSyncJobs != NULL)
		{
			LOG_LOGD("Package ID [%s]", pkgId.c_str());

			map<int, ISyncJob*>& allSyncJobs = pPackageSyncJobs->GetAllSyncJobs();
			if(!allSyncJobs.empty())
			{
				g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
				LOG_LOGD("Package has [%d] sync jobs", allSyncJobs.size());
				map< int, ISyncJob* >::iterator itr = allSyncJobs.begin();
				while (itr != allSyncJobs.end())
				{
					ISyncJob* syncJob = itr->second;
					g_variant_builder_add_value(&builder, marshal_sync_job(syncJob));
					itr++;
				}

				outSyncJobList = g_variant_builder_end(&builder);
				ret = SYNC_ERROR_NONE;
			}
			else
			{
				LOG_LOGD("Package has no pending sync jobs");
			}

		}
	}

	if (ret != SYNC_ERROR_NONE)
	{
		GError* error = g_error_new (_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_get_all_sync_jobs(pObject, pInvocation, outSyncJobList);

	LOG_LOGD("End");

	return true;
}


gboolean
sync_manager_set_sync_status( TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, gboolean sync_enable)
{
	SyncManager::GetInstance()->SetSyncSetting(sync_enable);

	tizen_sync_manager_complete_set_sync_status(pObject, pInvocation);
	return true;
}


/*
 * DBus related initialization and setup
 */
static void
OnBusAcquired (GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
	GDBusInterfaceSkeleton* interface = NULL;
	sync_ipc_obj = tizen_sync_manager_skeleton_new();
	if (sync_ipc_obj == NULL)
	{
		LOG_LOGD("sync_ipc_obj NULL!!");
		return;
	}
	interface = G_DBUS_INTERFACE_SKELETON(sync_ipc_obj);
	if (!g_dbus_interface_skeleton_export(interface, pConnection, SYNC_MANAGER_DBUS_PATH, NULL))
	{
		LOG_LOGD("export failed!!");
		return;
	}
	g_signal_connect(sync_ipc_obj, "handle-add-sync-adapter", G_CALLBACK(sync_manager_add_sync_adapter), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-sync-adapter", G_CALLBACK(sync_manager_remove_sync_adapter), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-on-demand-sync-job", G_CALLBACK(sync_manager_add_on_demand_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-periodic-sync-job", G_CALLBACK(sync_manager_add_periodic_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-data-change-sync-job", G_CALLBACK(sync_manager_add_data_change_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-get-all-sync-jobs", G_CALLBACK(sync_manager_get_all_sync_jobs), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-sync-job", G_CALLBACK(sync_manager_remove_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-set-sync-status", G_CALLBACK(sync_manager_set_sync_status), NULL);

	gdbusConnection = pConnection;
	LOG_LOGD("Sync Service started [%s]", pName);

	//g_dbus_object_manager_server_set_connection(pServerManager, connection);
}


static void
OnNameAcquired (GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
}


static void
OnNameLost (GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
	LOG_LOGD("OnNameLost");
	//exit (1);
}


static bool
__initialize_dbus()
{
	static guint ownerId = g_bus_own_name (G_BUS_TYPE_SESSION,
			"org.tizen.sync",
			G_BUS_NAME_OWNER_FLAGS_NONE,
			OnBusAcquired,
			OnNameAcquired,
			OnNameLost,
			NULL,
			NULL);

	if (ownerId == 0)
	{
		LOG_LOGD("gdbus own failed!!");
		return false;
	}

	return true;
}


void
SyncService::InitializeDbus(void)
{
	LOG_LOGD("Dbus initialization starts");

	if (__initialize_dbus() == false)
	{
		LOG_LOGD("__initialize_dbus failed");
		exit(1);
	}

	g_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
}


/*
 * DBus related initialization done
 */
SyncService::SyncService(void)
{
	LOG_LOGD("Sync service initialization starts");

	InitializeDbus();
}


SyncService::~SyncService(void)
{
}

//}//_SyncManager
