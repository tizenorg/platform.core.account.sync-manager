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
#include <app_manager.h>
#include <pkgmgr-info.h>
#include <Elementary.h>
#include <cynara-client.h>
#include <cynara-session.h>
#include <cynara-creds-gdbus.h>
#include "sync-error.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_ServiceInterface.h"
#include "sync-manager-stub.h"
#include "sync-ipc-marshal.h"
#include "sync-adapter-stub.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncJobsInfo.h"
#include "SyncManager_SyncAdapterAggregator.h"
#include "SyncManager_SyncJobsAggregator.h"


static GDBusConnection *gdbusConnection = NULL;
static cynara *pCynara;
static bool check_jobs = false;

/*namespace _SyncManager
{*/

#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_DBUS_PATH "/org/tizen/sync/adapter/"
#define SYNC_ERROR_DOMAIN "sync-manager"
#define SYNC_ERROR_PREFIX "org.tizen.sync.Error"
#define PRIV_ALARM_SET "http://tizen.org/privilege/alarm.set"

#if defined(_SEC_FEATURE_CALENDAR_CONTACTS_ENABLE)
#define PRIV_CALENDAR_READ "http://tizen.org/privilege/calendar.read"
#define PRIV_CONTACT_READ "http://tizen.org/privilege/contact.read"
#endif


#define SYS_DBUS_INTERFACE				"org.tizen.system.deviced.PowerOff"
#define SYS_DBUS_MATCH_RULE				"type='signal',interface='org.tizen.system.deviced.PowerOff'"
#define SYS_DBUS_PATH					"/Org/Tizen/System/DeviceD/PowerOff"
#define POWEROFF_MSG					"ChangeState"

bool ShutdownInitiated = false;
static TizenSyncManager* sync_ipc_obj = NULL;
GHashTable* g_hash_table = NULL;
string sa_app_id;
static guint signal_id = -1;

using namespace std;

void convert_to_path(char app_id[])
{
	int i = 0;
	while (app_id[i] != '\0') {
		if (app_id[i] == '.' || app_id[i] == '-') {
			app_id[i] = '_';
		}
		i++;
	}
}


int
SyncService::StartService()
{
	__pSyncManagerInstance = SyncManager::GetInstance();
	if (__pSyncManagerInstance == NULL) {
		LOG_LOGD("Failed to initialize sync manager");
		return -1;
	}

	bool ret = __pSyncManagerInstance->Construct();
	if (!ret) {
		LOG_LOGD("Sync Manager Construct failed");
		return -1;
	}

	int cynara_result = cynara_initialize(&pCynara, NULL);
	if (cynara_result != CYNARA_API_SUCCESS) {
		LOG_LOGD("Cynara Initialization is failed");
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
_sync_error_quark(void)
{
	static volatile gsize quark_volatile = 0;

	g_dbus_error_register_error_domain(SYNC_ERROR_DOMAIN,
										&quark_volatile,
										_sync_errors,
										G_N_ELEMENTS(_sync_errors));

	return (GQuark) quark_volatile;
}


static guint
get_caller_pid(GDBusMethodInvocation* pMethodInvocation)
{
	guint pid = -1;
	const char *name = NULL;
	name = g_dbus_method_invocation_get_sender(pMethodInvocation);
	if (name == NULL) {
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

	if (ret != NULL) {
		g_variant_get(ret, "(u)", &pid);
		g_variant_unref(ret);
	}

	return pid;
}


static int
__get_data_for_checking_privilege(GDBusMethodInvocation *invocation, char **client, char **session, char **user)
{
	GDBusConnection *gdbus_conn = NULL;
	char *sender = NULL;
	int ret = CYNARA_API_SUCCESS;

	gdbus_conn = g_dbus_method_invocation_get_connection(invocation);
	if (gdbus_conn == NULL) {
		LOG_LOGD("sync service: g_dbus_method_invocation_get_connection failed");
		return -1;
	}

	sender = (char*) g_dbus_method_invocation_get_sender(invocation);
	if (sender == NULL) {
		LOG_LOGD("sync service: g_dbus_method_invocation_get_sender failed");
		return -1;
	}

	ret = cynara_creds_gdbus_get_user(gdbus_conn, sender, USER_METHOD_DEFAULT, user);
	if (ret != CYNARA_API_SUCCESS) {
		LOG_LOGD("sync service: cynara_creds_gdbus_get_user failed, ret = %d", ret);
		return -1;
	}

	ret = cynara_creds_gdbus_get_client(gdbus_conn, sender, CLIENT_METHOD_DEFAULT, client);
	if (ret != CYNARA_API_SUCCESS) {
		LOG_LOGD("sync service: cynara_creds_gdbus_get_client failed, ret = %d", ret);
		return -1;
	}

	guint pid = get_caller_pid(invocation);
	LOG_LOGD("client Id = [%u]", pid);

	*session = cynara_session_from_pid(pid);
	if (*session == NULL) {
		LOG_LOGD("sync service: cynara_session_from_pid failed");
		return -1;
	}

	return SYNC_ERROR_NONE;
}


static int
__check_privilege_by_cynara(const char *client, const char *session, const char *user, const char *privilege)
{
	int ret = CYNARA_API_ACCESS_ALLOWED;
	char err_buf[128] = {0, };

	ret = cynara_check(pCynara, client, session, user, privilege);
	switch (ret) {
		case CYNARA_API_ACCESS_ALLOWED:
			LOG_LOGD("sync service: cynara_check success, privilege [%s], result [CYNARA_API_ACCESS_ALLOWED]", privilege);
			return SYNC_ERROR_NONE;
		case CYNARA_API_ACCESS_DENIED:
			LOG_LOGD("sync service: cynara_check failed, privilege [%s], result [CYNARA_API_ACCESS_DENIED]", privilege);
			return SYNC_ERROR_PERMISSION_DENIED;
		default:
			cynara_strerror(ret, err_buf, sizeof(err_buf));
			LOG_LOGD("sync service: cynara_check error [%s], privilege [%s] is required", err_buf, privilege);
			return SYNC_ERROR_PERMISSION_DENIED;
	}
}


int
_check_privilege(GDBusMethodInvocation *invocation, const char *privilege)
{
	int ret = SYNC_ERROR_NONE;
	char *client = NULL;
	char *session = NULL;
	char *user = NULL;

	ret = __get_data_for_checking_privilege(invocation, &client, &session, &user);
	if (ret != SYNC_ERROR_NONE) {
		LOG_LOGD("sync service: __get_data_for_checking_privilege is failed, ret [%d]", ret);
		g_free(client);
		g_free(user);
		if (session != NULL)
			g_free(session);
		return SYNC_ERROR_PERMISSION_DENIED;
	}

	ret = __check_privilege_by_cynara(client, session, user, privilege);
	if (ret != SYNC_ERROR_NONE) {
		LOG_LOGD("sync service: __check_privilege_by_cynara is failed, ret [%d]", ret);
		g_free(client);
		g_free(user);
		if (session != NULL)
			g_free(session);
		return SYNC_ERROR_PERMISSION_DENIED;
	}

	g_free(client);
	g_free(user);
	if (session != NULL)
		g_free(session);

	return SYNC_ERROR_NONE;
}


int _check_privilege_alarm_set(GDBusMethodInvocation *invocation)
{
	return _check_privilege(invocation, PRIV_ALARM_SET);
}


#if defined(_SEC_FEATURE_CALENDAR_CONTACTS_ENABLE)
int _check_privilege_calendar_read(GDBusMethodInvocation *invocation)
{
	return _check_privilege(invocation, PRIV_CALENDAR_READ);
}


int _check_privilege_contact_read(GDBusMethodInvocation *invocation)
{
	return _check_privilege(invocation, PRIV_CONTACT_READ);
}
#endif


int
SyncService::TriggerStartSync(const char* appId, int accountId, const char* syncJobName, bool isDataSync, bundle* pExtras)
{
	LOG_LOGD("appId [%s] jobname [%s]", appId, syncJobName);

	app_control_h app_control;
	int ret = SYNC_ERROR_NONE;
	GError *error = NULL;
	guint pid = -1;

	app_context_h app_context = NULL;
	ret = app_manager_get_app_context(appId, &app_context);
	if (ret == APP_MANAGER_ERROR_NO_SUCH_APP) {
		gboolean alreadyRunning = FALSE;
		GVariant *result = g_dbus_connection_call_sync(gdbusConnection,	"org.freedesktop.DBus",
														"/org/freedesktop/DBus",
														"org.freedesktop.DBus",
														"NameHasOwner",
														g_variant_new("(s)", appId),
														NULL,
														G_DBUS_CALL_FLAGS_NONE,
														-1,
														NULL,
														&error);
		if (result == NULL) {
			LOG_LOGD("g_dbus_connection_call_sync() is failed");
			if (error) {
				LOG_LOGD("dbus error message: %s", error->message);
				g_error_free(error);
			}
		} else {
			g_variant_get(result, "(b)", &alreadyRunning);
		}

		if (!alreadyRunning) {
			LOG_LOGD("Service not running. Start the service ");
			GVariant *ret = g_dbus_connection_call_sync(gdbusConnection,	"org.freedesktop.DBus",
														"/org/freedesktop/DBus",
														"org.freedesktop.DBus",
														"StartServiceByName",
														g_variant_new("(su)", appId),
														NULL,
														G_DBUS_CALL_FLAGS_NONE,
														-1,
														NULL,
														&error);
			if (ret != NULL) {
				g_variant_get(ret, "(u)", &pid);
				g_variant_unref(ret);
			}

			if (error) {
				LOG_LOGD("g_dbus_connection_call_sync gdbus error [%s]", error->message);
				g_clear_error(&error);
			}
			return SYNC_ERROR_SYSTEM;
		}
	}
	else {
		bool isRunning = false;
		app_manager_is_running(appId, &isRunning);
		if (!isRunning) {
			LOG_LOGD("app is not running, launch the app and wait for signal");
			ret = app_control_create(&app_control);
			SYNC_LOGE_RET_RES(ret == APP_CONTROL_ERROR_NONE, SYNC_ERROR_SYSTEM, "app control create failed %d", ret);

			ret = app_control_set_app_id(app_control, appId);
			if (ret != APP_CONTROL_ERROR_NONE) {
				LOG_LOGD("app control error %d", ret);
				app_control_destroy(app_control);
				return SYNC_ERROR_SYSTEM;
			}

			sa_app_id.clear();
			ret = app_control_send_launch_request(app_control, NULL, NULL);
			SYNC_LOGE_RET_RES(ret == APP_CONTROL_ERROR_NONE, SYNC_ERROR_SYSTEM, "app control launch request failed %d", ret);
		}
		else {
			LOG_LOGD("app is already running");
		}

		app_context_destroy(app_context);
	}

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter) {
		GVariant* pBundle = marshal_bundle(pExtras);
		tizen_sync_adapter_emit_start_sync(pSyncAdapter, accountId, syncJobName, isDataSync, pBundle);
	}
	else {
		LOG_LOGD("Sync adapter entry not found");
		return SYNC_ERROR_SYSTEM;
	}

	return SYNC_ERROR_NONE;
}


void
SyncService::TriggerStopSync(const char* appId, int accountId, const char* syncJobName, bool isDataSync, bundle* pExtras)
{
	LOG_LOGD("Trigger stop sync %s", appId);

	//int id = -1;

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter == NULL) {
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

	SyncJobsAggregator* pSyncJobsAggregator = __pSyncManagerInstance->GetSyncJobsAggregator();

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob) {
		SyncJob* pSyncJobEntry = dynamic_cast< SyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get sync job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption);
		LOG_LOGD("sync parameters are updated with new parameters", pSyncJobName);
	}
	else {
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncManagerInstance->AddOnDemandSync(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId);
	}

	if (ret == SYNC_ERROR_NONE) {
		*pSyncJobId = syncJobId;
	}

	return ret;
}


int
SyncService::RequestPeriodicSync(const char* pPackageId, const char* pSyncJobName, int accountId, bundle* pExtras, int syncOption, unsigned long pollFrequency, int* pSyncJobId)
{
	int ret = SYNC_ERROR_NONE;
	SyncJobsAggregator* pSyncJobsAggregator = __pSyncManagerInstance->GetSyncJobsAggregator();
	int syncJobId = -1;

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob) {
		PeriodicSyncJob* pSyncJobEntry = dynamic_cast< PeriodicSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get syn job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption, pollFrequency);
		LOG_LOGD("sync jobs is updated with new parameters");

		if (pSyncJobEntry->IsExpedited()) {
			LOG_LOGD("sync request with priority. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
			SyncManager::GetInstance()->ScheduleSyncJob(pSyncJobEntry);
		}
	} else {
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncManagerInstance->AddPeriodicSyncJob(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId, pollFrequency);
	}

	if (ret == SYNC_ERROR_NONE) {
		*pSyncJobId = syncJobId;
	}

	return ret;
}


int
SyncService::RequestDataSync(const char* pPackageId, const char* pSyncJobName, int accountId, bundle* pExtras, int syncOption, const char* pCapability, int* pSyncJobId)
{
	int ret = SYNC_ERROR_NONE;
	SyncJobsAggregator* pSyncJobsAggregator = __pSyncManagerInstance->GetSyncJobsAggregator();
	int syncJobId = -1;

	ISyncJob* pSyncJob = pSyncJobsAggregator->GetSyncJob(pPackageId, pSyncJobName);
	if (pSyncJob) {
		DataSyncJob* pSyncJobEntry = dynamic_cast< DataSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(pSyncJobEntry != NULL, SYNC_ERROR_SYSTEM, "Failed to get syn job");

		syncJobId = pSyncJobEntry->GetSyncJobId();
		LOG_LOGD("Sync request with job name [%s] already found. Sync job id [%d]", pSyncJobName, syncJobId);

		pSyncJobEntry->Reset(accountId, pExtras, syncOption, pCapability);
		LOG_LOGD("sync jobs is updated with new parameters");

		if (pSyncJobEntry->IsExpedited()) {
			LOG_LOGD("sync request with priority. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
			SyncManager::GetInstance()->ScheduleSyncJob(pSyncJobEntry);
		}
	}
	else {
		syncJobId = pSyncJobsAggregator->GenerateSyncJobId(pPackageId);
		SYNC_LOGE_RET_RES(syncJobId <= SYNC_JOB_LIMIT, SYNC_ERROR_QUOTA_EXCEEDED, "Sync job quota exceeded");

		LOG_LOGD("New sync request. Adding sync job with Sync job name [%s] Sync job id [%d]", pSyncJobName, syncJobId);
		ret = __pSyncManagerInstance->AddDataSyncJob(pPackageId, pSyncJobName, accountId, pExtras, syncOption, syncJobId, pCapability);
	}

	if (ret == SYNC_ERROR_NONE) {
		*pSyncJobId = syncJobId;
	}
	return ret;
}


void
SyncService::HandleShutdown(void)
{
	LOG_LOGD("Shutdown Starts");

	cynara_finish(pCynara);

	SyncManager::GetInstance()->HandleShutdown();

	LOG_LOGD("Shutdown Ends");
}


int
get_service_name_by_pid(guint pid, char** pAppId)
{
		GError* error = NULL;
		GVariant *unit = NULL;
		unit = g_dbus_connection_call_sync(gdbusConnection,	"org.freedesktop.systemd1",
															"/org/freedesktop/systemd1",
															"org.freedesktop.systemd1.Manager",
															"GetUnitByPID",
															g_variant_new("(u)", pid),
															G_VARIANT_TYPE("(o)"),
															G_DBUS_CALL_FLAGS_NONE,
															-1,
															NULL,
															&error);
		if (error) {
			LOG_LOGC("get_service_name_by_pid gdbus error [%s]", error->message);
			*pAppId = NULL;
			g_clear_error(&error);
			return SYNC_ERROR_SYSTEM;
		}

		char* unitNode = NULL;
		g_variant_get(unit, "(o)", &unitNode);

		GVariant *service = NULL;
		service = g_dbus_connection_call_sync(gdbusConnection,	"org.freedesktop.systemd1",
																		unitNode,
																		"org.freedesktop.DBus.Properties",
																		"Get",
																		g_variant_new("(ss)", "org.freedesktop.systemd1.Service", "BusName"),
																		G_VARIANT_TYPE("(v)"),
																		G_DBUS_CALL_FLAGS_NONE,
																		-1,
																		NULL,
																		&error);
		if (error) {
			LOG_LOGC("get_service_name_by_pid gdbus error [%s]", error->message);
			*pAppId = NULL;
			g_clear_error(&error);
			return SYNC_ERROR_SYSTEM;
		}

		GVariant *tmp = g_variant_get_child_value(service, 0);
		if (tmp) {
			GVariant* va = g_variant_get_variant(tmp);
			if (va) {
				char* name = NULL;
				g_variant_get(va, "s", &name);
				*pAppId = strdup(name);
				LOG_LOGD("DBus service [%s]", *pAppId);
			}
		}

		return SYNC_ERROR_NONE;
}


int
get_num_of_sync_jobs(string pkgId)
{
	if (!pkgId.empty()) {
		SyncJobsAggregator* pSyncJobsAggregator = SyncManager::GetInstance()->GetSyncJobsAggregator();
		SyncJobsInfo* pPackageSyncJobs = pSyncJobsAggregator->GetSyncJobsInfo(pkgId.c_str());

		if (pPackageSyncJobs != NULL) {
			map<int, ISyncJob*>& allSyncJobs = pPackageSyncJobs->GetAllSyncJobs();
			if (!allSyncJobs.empty())
				LOG_LOGD("Package ID [%s], it has [%d] sync job", pkgId.c_str(), allSyncJobs.size());
			else {
				LOG_LOGD("Package ID [%s], doesn't have any sync job", pkgId.c_str());
				return 0;
			}

			return allSyncJobs.size();
		}
	}

	return 0;
}


/*
* org.tizen.sync.adapter interface methods
*/
gboolean
sync_adapter_handle_send_result(TizenSyncAdapter* pObject, GDBusMethodInvocation* pInvocation,
																const gchar* pCommandLine,
																gint sync_result,
																const gchar* sync_job_name)
{
	LOG_LOGD("Received sync job result");
	guint pid = get_caller_pid(pInvocation);

	string pkgIdStr;
	char* pAppId = NULL;
	int ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
	}
	else {
		ret = get_service_name_by_pid(pid, &pAppId);
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	if (!pkgIdStr.empty()) {
		LOG_LOGD("Sync result received from [%s]: sync_job_name [%s] result [%d]", pAppId, sync_job_name, sync_result);

		SyncManager::GetInstance()->OnResultReceived((SyncStatus)sync_result, pAppId, pkgIdStr, sync_job_name);
		free(pAppId);
	}
	else
		LOG_LOGD("sync service: Get package Id fail %d", ret);

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
	if (ret != APP_MANAGER_ERROR_NONE)
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
sync_manager_add_on_demand_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																const gchar* pCommandLine,
																gint accountId,
																const gchar* pSyncJobName,
																gint sync_option,
																GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received On-Demand Sync request");

	guint pid = get_caller_pid(pInvocation);
	string pkgIdStr;
	char* pAppId = NULL;
	int ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
		free(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request %s", pCommandLine);
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	int sync_job_id = 0;
	if (!pkgIdStr.empty()) {
		LOG_LOGD("Params acc[%d] name[%s] option[%d] package[%s]", accountId, pSyncJobName, sync_option, pkgIdStr.c_str());
		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestOnDemandSync(pkgIdStr.c_str(), pSyncJobName, accountId, pBundle, sync_option, &sync_job_id);
		bundle_free(pBundle);
	}
	else {
		LOG_LOGD("Failed to get package id");
		ret = SYNC_ERROR_SYSTEM;
	}

	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_on_demand_job(pObject, pInvocation, sync_job_id);

	ManageIdleState* pManageIdleState = SyncManager::GetInstance()->GetManageIdleState();

	if (get_num_of_sync_jobs(pkgIdStr) <= 1)
		pManageIdleState->ResetTermTimer();

	LOG_LOGD("End of On-Demand Sync request");

	return true;
}


gboolean
sync_manager_remove_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine, gint sync_job_id)
{
	LOG_LOGD("Request to remove sync job %d", sync_job_id);

	guint pid = get_caller_pid(pInvocation);
	string pkgIdStr;
	char* pAppId;
	int ret = APP_MANAGER_ERROR_NONE;

	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
		free(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}
	if (!pkgIdStr.empty()) {
		LOG_LOGD("package id [%s]", pkgIdStr.c_str());
		ret = SyncManager::GetInstance()->RemoveSyncJob(pkgIdStr, sync_job_id);
	}
	else
		LOG_LOGD("sync service: Get package Id fail %d", ret);

	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_remove_job(pObject, pInvocation);

	ManageIdleState* pManageIdleState = SyncManager::GetInstance()->GetManageIdleState();

	if (get_num_of_sync_jobs(pkgIdStr) == 0)
		pManageIdleState->SetTermTimer();

	LOG_LOGD("sync service: remove sync job ends");

	return true;
}


gboolean
sync_manager_add_periodic_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																const gchar* pCommandLine,
																gint accountId,
																const gchar* pSyncJobName,
																gint sync_interval,
																gint sync_option,
																GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received Period Sync request");

	int ret = SYNC_ERROR_NONE;

	ret = _check_privilege_alarm_set(pInvocation);
	if (ret != SYNC_ERROR_NONE) {
		GError *error = g_error_new(_sync_error_quark(), ret, "sync service: alarm.set permission denied");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
		return true;
	}

	guint pid = get_caller_pid(pInvocation);
	string pkgIdStr;
	int sync_job_id = 0;
	char* pAppId;

	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
		free(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}
	if (!pkgIdStr.empty()) {
		LOG_LOGD("Params acc[%d] name[%s] option[%d] period[%d] package[%s]", accountId, pSyncJobName, sync_option, sync_interval, pkgIdStr.c_str());
		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestPeriodicSync(pkgIdStr.c_str(), pSyncJobName, accountId, pBundle, sync_option, sync_interval, &sync_job_id);
		bundle_free(pBundle);
	}
	else
		LOG_LOGD("sync service: Get package Id fail %d", ret);

	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_periodic_job(pObject, pInvocation, sync_job_id);

	ManageIdleState* pManageIdleState = SyncManager::GetInstance()->GetManageIdleState();

	int num_sync_job = get_num_of_sync_jobs(pkgIdStr);
	if (num_sync_job >= 1)
		pManageIdleState->UnsetTermTimer();
	else if (num_sync_job == 0)
		pManageIdleState->ResetTermTimer();

	LOG_LOGD("sync service: add periodic sync job ends");

	return true;
}


gboolean
sync_manager_add_data_change_job(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation,
																	const gchar* pCommandLine,
																	gint accountId,
																	const gchar* pCapabilityArg,
																	gint sync_option,
																	GVariant* pSyncJobUserData)
{
	LOG_LOGD("Received data change Sync request");

	int ret = SYNC_ERROR_NONE;

	const char *capability = (char *)pCapabilityArg;
#if defined(_SEC_FEATURE_CALENDAR_CONTACTS_ENABLE)
	if (!strcmp(capability, "http://tizen.org/sync/capability/calendar") ||
		!strcmp(capability, "http://tizen.org/sync/capability/contact")) {
		if (!strcmp(capability, "http://tizen.org/sync/capability/calendar"))
			ret = _check_privilege_calendar_read(pInvocation);
		else
			ret = _check_privilege_contact_read(pInvocation);

		if (ret != SYNC_ERROR_NONE) {
			GError* error = NULL;
			if (!strcmp(capability, "http://tizen.org/sync/capability/calendar"))
				error = g_error_new(_sync_error_quark(), ret, "sync service: calendar.read permission denied");
			else
				error = g_error_new(_sync_error_quark(), ret, "sync service: contact.read permission denied");

			g_dbus_method_invocation_return_gerror(pInvocation, error);
			g_clear_error(&error);
			return true;
		}
	}
#endif

	guint pid = get_caller_pid(pInvocation);
	string pkgIdStr;
	int sync_job_id = 0;
	char* pAppId;

	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
		free(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}
	if (!pkgIdStr.empty()) {
		LOG_LOGD("Params account [%d] job_name [%s] sync_option[%d] sync_job_id[%d] package [%s] ", accountId, pCapabilityArg, sync_option, sync_job_id, pkgIdStr.c_str());

		bundle* pBundle = umarshal_bundle(pSyncJobUserData);
		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		ret = SyncService::GetInstance()->RequestDataSync(pkgIdStr.c_str(), pCapabilityArg, accountId, pBundle, sync_option, pCapabilityArg, &sync_job_id);

		bundle_free(pBundle);
	}
	else
		LOG_LOGD("sync service: Get package Id fail %d", ret);

	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_data_change_job(pObject, pInvocation, sync_job_id);

	ManageIdleState* pManageIdleState = SyncManager::GetInstance()->GetManageIdleState();

	int num_sync_job = get_num_of_sync_jobs(pkgIdStr);
	if (num_sync_job >= 1)
		pManageIdleState->UnsetTermTimer();
	else if (num_sync_job == 0)
		pManageIdleState->ResetTermTimer();

	LOG_LOGD("sync service: add data sync job ends");

	return true;
}


static bool
is_service_app(pid_t pid)
{
	char *current_app_id = NULL;
	int ret = app_manager_get_app_id(pid, &current_app_id);
	if (ret != APP_MANAGER_ERROR_NONE) {
		LOG_LOGD("Getting current app id is failed : %d, %s", ret, get_error_message(ret));
		return false;
	}

	pkgmgrinfo_appinfo_h current_app_info = NULL;

	ret = pkgmgrinfo_appinfo_get_appinfo(current_app_id, &current_app_info);
	if (ret != PMINFO_R_OK) {
		LOG_LOGD("Current app info handle creation error : %d, %s", ret, get_error_message(ret));
		free(current_app_id);
		return false;
	}
	char *current_app_type = NULL;
	ret = pkgmgrinfo_appinfo_get_component_type(current_app_info, &current_app_type);
	if (ret != PMINFO_R_OK) {
		LOG_LOGD("Current app info getting app type error : %d, %s", ret, get_error_message(ret));

		pkgmgrinfo_appinfo_destroy_appinfo(current_app_info);
		free(current_app_id);
		return false;
	}
	else {
		if (!strcmp(current_app_type, "svcapp")) {
			LOG_LOGD("Current application type : %s", current_app_type);
			pkgmgrinfo_appinfo_destroy_appinfo(current_app_info);
		}
		else {
			LOG_LOGD("Current app is not a service application : %s", current_app_type);
			pkgmgrinfo_appinfo_destroy_appinfo(current_app_info);
			free(current_app_id);
			return false;
		}
	}

	free(current_app_id);

	return true;
}


static inline int __read_proc(const char *path, char *buf, int size)
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


gboolean
sync_manager_add_sync_adapter(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine)
{
	LOG_LOGD("Received sync adapter registration request");

	guint pid = get_caller_pid(pInvocation);

	if (!is_service_app(pid)) {
		GError* error = g_error_new(_sync_error_quark(), SYNC_ERROR_INVALID_OPERATION, "App not supported");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
		return false;
	}

	int ret = SYNC_ERROR_SYSTEM;
	string pkgIdStr;
	char* pAppId = NULL;
	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
	}
	else {
		ret = get_service_name_by_pid(pid, &pAppId);
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	if (!pkgIdStr.empty()) {
		SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();
		if (pAggregator == NULL) {
			LOG_LOGD("sync adapter aggregator is NULL");
			tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);
			return true;
		}
		if (pAggregator->HasSyncAdapter(pkgIdStr.c_str())) {
			const char *registered_app_id = pAggregator->GetSyncAdapter(pkgIdStr.c_str());
			LOG_LOGD("registered appId is [%s]", registered_app_id);
			LOG_LOGD("caller appId is [%s]", pAppId);
			if (strcmp(pAppId, registered_app_id)) {
				GError* error = g_error_new(_sync_error_quark(), SYNC_ERROR_QUOTA_EXCEEDED, "Sync adapter already registered");
				g_dbus_method_invocation_return_gerror(pInvocation, error);
				g_clear_error(&error);
				return false;
			}
			else {
				check_jobs = true; // Probably sync service may have started this service. Alert sync manager for scheduling pending jobs.
			}
		}

		char object_path[50];
		snprintf(object_path, 50, "%s%d", SYNC_ADAPTER_DBUS_PATH, pid);

		GError *error = NULL;
		GDBusInterfaceSkeleton* interface = NULL;
		TizenSyncAdapter* syncAdapterObj = tizen_sync_adapter_skeleton_new();
		if (syncAdapterObj != NULL) {
			interface = G_DBUS_INTERFACE_SKELETON(syncAdapterObj);
			if (g_dbus_interface_skeleton_export(interface, gdbusConnection, object_path, &error)) {
				g_signal_connect(syncAdapterObj, "handle-send-result", G_CALLBACK(sync_adapter_handle_send_result), NULL);
				pAggregator->AddSyncAdapter(pkgIdStr.c_str(), pAppId);

				LOG_LOGD("inserting sync adapter ipc %s", pAppId);
				g_hash_table_insert(g_hash_table, strdup(pAppId), syncAdapterObj);

				ret = SYNC_ERROR_NONE;
			}
			else {
				LOG_LOGD("export failed %s", error->message);
			}
		}
		else {
			LOG_LOGD("sync adapter object creation failed");
		}
	}
	else {
		LOG_LOGD("Failed to get package ID");
	}

	if (ret != SYNC_ERROR_NONE) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);

	if (check_jobs) {
		SyncManager::GetInstance()->AlertForChange();
	}

	LOG_LOGD("sync service: adding sync adapter ends");

	return true;
}


gboolean
sync_manager_remove_sync_adapter(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine)
{
	LOG_LOGD("Request to remove sync adapter");
	string pkgIdStr;

	guint pid = get_caller_pid(pInvocation);
	if (!is_service_app(pid)) {
		GError* error = g_error_new(_sync_error_quark(), SYNC_ERROR_INVALID_OPERATION, "App not supported");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
		return false;
	}

	char* pAppId;
	int ret = APP_MANAGER_ERROR_NONE;

	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgIdStr = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();

	if (!pkgIdStr.empty()) {
		pAggregator->RemoveSyncAdapter(pkgIdStr.c_str());
		LOG_LOGD("Sync adapter removed for package [%s]", pkgIdStr.c_str());
	}
	else
		LOG_LOGD("sync service: Get package Id fail %d", ret);

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, pAppId);
	if (pSyncAdapter == NULL) {
		LOG_LOGD("Failed to lookup syncadapter gdbus object for [%s]", pAppId);
		char object_path[50];
		snprintf(object_path, 50, "%s%d", SYNC_ADAPTER_DBUS_PATH, pid);

		GError *error = NULL;
		GDBusInterfaceSkeleton* interface = NULL;
		TizenSyncAdapter* syncAdapterObj = tizen_sync_adapter_skeleton_new();
		if (syncAdapterObj != NULL) {
			interface = G_DBUS_INTERFACE_SKELETON(syncAdapterObj);
			if (g_dbus_interface_skeleton_export(interface, gdbusConnection, object_path, &error)) {
				g_signal_connect(syncAdapterObj, "handle-send-result", G_CALLBACK(sync_adapter_handle_send_result), NULL);
				pAggregator->RemoveSyncAdapter(pkgIdStr.c_str());

				LOG_LOGD("deletting sync adapter ipc %s", pAppId);

				ret = SYNC_ERROR_NONE;
			}
			else {
				LOG_LOGD("export failed %s", error->message);
			}
		}

		free(pAppId);
	}
	else {
		GDBusInterfaceSkeleton* interface = NULL;
		interface = G_DBUS_INTERFACE_SKELETON(pSyncAdapter);
		g_dbus_interface_skeleton_unexport(interface);
	}

	tizen_sync_manager_complete_remove_sync_adapter(pObject, pInvocation);

	LOG_LOGD("sync service: removing sync adapter ends");

	return true;
}


GVariant *
marshal_sync_job(ISyncJob* syncJob)
{
	SyncJob* pSyncJob = dynamic_cast< SyncJob* > (syncJob);
	GVariantBuilder builder;

	if (pSyncJob) {
		g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_ID, g_variant_new_int32(pSyncJob->GetSyncJobId()));
		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_ACC_ID, g_variant_new_int32(pSyncJob->__accountId));

		if (pSyncJob->GetSyncType() == SYNC_TYPE_DATA_CHANGE)
			g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_CAPABILITY, g_variant_new_string(pSyncJob->__syncJobName.c_str()));
		else
			g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_NAME, g_variant_new_string(pSyncJob->__syncJobName.c_str()));

		//LOG_LOGD("job name and id [%s] [%d]", pSyncJob->__syncJobName.c_str(), pSyncJob->GetSyncJobId());

		g_variant_builder_add(&builder, "{sv}", KEY_SYNC_JOB_USER_DATA, marshal_bundle(pSyncJob->__pExtras));
	}

	return g_variant_builder_end (&builder);
}



gboolean
sync_manager_get_all_sync_jobs(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, const gchar* pCommandLine)
{
	LOG_LOGD("Received request to get Sync job ids");

	guint pid = get_caller_pid(pInvocation);
	string pkgId;
	char* pAppId;
	int ret = APP_MANAGER_ERROR_NONE;

	ret = app_manager_get_app_id(pid, &pAppId);
	if (ret == APP_MANAGER_ERROR_NONE) {
		pkgId = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
		free(pAppId);
	}
	else {
		LOG_LOGD("Request seems to be from app-id less/command line based request");
		pkgId = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	GVariant* outSyncJobList = NULL;
	GVariantBuilder builder;

	if (!pkgId.empty()) {
		SyncJobsAggregator* pSyncJobsAggregator = SyncManager::GetInstance()->GetSyncJobsAggregator();
		SyncJobsInfo* pPackageSyncJobs = pSyncJobsAggregator->GetSyncJobsInfo(pkgId.c_str());

		if (pPackageSyncJobs != NULL) {
			LOG_LOGD("Package ID [%s]", pkgId.c_str());

			map<int, ISyncJob*>& allSyncJobs = pPackageSyncJobs->GetAllSyncJobs();
			if (!allSyncJobs.empty()) {
				g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
				LOG_LOGD("Package has [%d] sync jobs", allSyncJobs.size());
				map< int, ISyncJob* >::iterator itr = allSyncJobs.begin();
				while (itr != allSyncJobs.end()) {
					ISyncJob* syncJob = itr->second;
					g_variant_builder_add_value(&builder, marshal_sync_job(syncJob));
					itr++;
				}
				outSyncJobList = g_variant_builder_end(&builder);
			}
		}
		else {
			LOG_LOGD("sync service: registered sync jobs are not found");
		}
	}
	else {
		LOG_LOGD("Sync jobs information not found for package [%s], pkgId.str()");
	}

	if (outSyncJobList == NULL) {
		GError* error = g_error_new(_sync_error_quark(), ret, "system error");
		g_dbus_method_invocation_return_gerror(pInvocation, error);
		g_clear_error(&error);
	}
	else
		tizen_sync_manager_complete_get_all_sync_jobs(pObject, pInvocation, outSyncJobList);

	ManageIdleState* pManageIdleState = SyncManager::GetInstance()->GetManageIdleState();

	if (pManageIdleState->GetTermTimerId() != -1)
		pManageIdleState->ResetTermTimer();

	LOG_LOGD("sync service: get all sync jobs ends");

	return true;
}


gboolean
sync_manager_set_sync_status(TizenSyncManager* pObject, GDBusMethodInvocation* pInvocation, gboolean sync_enable)
{
	SyncManager::GetInstance()->SetSyncSetting(sync_enable);

	tizen_sync_manager_complete_set_sync_status(pObject, pInvocation);
	return true;
}


void
DbusSignalHandler(GDBusConnection* pConnection,
		const gchar* pSenderName,
		const gchar* pObjectPath,
		const gchar* pInterfaceName,
		const gchar* pSignalName,
		GVariant* pParameters,
		gpointer data)
{
	if (!(g_strcmp0(pObjectPath, SYS_DBUS_PATH)) && !(g_strcmp0(pSignalName, POWEROFF_MSG))) {
		LOG_LOGD("Shutdown dbus received");
		if (ShutdownInitiated == false) {
			ShutdownInitiated = true;
			sync_service_finalise();
			ecore_main_loop_quit();
		}
		if (pConnection && (signal_id != (unsigned int)-1)) {
			g_dbus_connection_signal_unsubscribe(pConnection, signal_id);
			g_object_unref(pConnection);
		}
	}
}


/*
 * DBus related initialization and setup
 */
static void
OnBusAcquired(GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
	GDBusInterfaceSkeleton* interface = NULL;
	sync_ipc_obj = tizen_sync_manager_skeleton_new();
	if (sync_ipc_obj == NULL) {
		LOG_LOGD("sync_ipc_obj NULL!!");
		return;
	}
	interface = G_DBUS_INTERFACE_SKELETON(sync_ipc_obj);
	if (!g_dbus_interface_skeleton_export(interface, pConnection, SYNC_MANAGER_DBUS_PATH, NULL)) {
		LOG_LOGD("export failed!!");
		return;
	}
	g_signal_connect(sync_ipc_obj, "handle-add-sync-adapter", G_CALLBACK(sync_manager_add_sync_adapter), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-sync-adapter", G_CALLBACK(sync_manager_remove_sync_adapter), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-on-demand-sync-job", G_CALLBACK(sync_manager_add_on_demand_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-periodic-sync-job", G_CALLBACK(sync_manager_add_periodic_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-data-change-sync-job", G_CALLBACK(sync_manager_add_data_change_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-get-all-sync-jobs", G_CALLBACK(sync_manager_get_all_sync_jobs), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-sync-job", G_CALLBACK(sync_manager_remove_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-set-sync-status", G_CALLBACK(sync_manager_set_sync_status), NULL);

	gdbusConnection = pConnection;

	signal_id = g_dbus_connection_signal_subscribe(pConnection, NULL, SYS_DBUS_INTERFACE, POWEROFF_MSG, SYS_DBUS_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE, DbusSignalHandler, NULL, NULL);
	if (signal_id == (unsigned int)-1) {
		LOG_LOGD("unable to register for PowerOff Signal");
		return;
	}
	LOG_LOGD("Sync Service started [%s]", pName);

	RepositoryEngine* __pSyncRepositoryEngine;
	__pSyncRepositoryEngine = RepositoryEngine::GetInstance();

	if (__pSyncRepositoryEngine == NULL)
		LOG_LOGD("Failed to construct RepositoryEngine");
	else
		__pSyncRepositoryEngine->OnBooting();
}


static void
OnNameAcquired(GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
}


static void
OnNameLost(GDBusConnection* pConnection, const gchar* pName, gpointer userData)
{
	LOG_LOGD("OnNameLost");
	//exit (1);
}


static bool
__initialize_dbus()
{
	static guint ownerId = g_bus_own_name(G_BUS_TYPE_SESSION,
			"org.tizen.sync",
			G_BUS_NAME_OWNER_FLAGS_NONE,
			OnBusAcquired,
			OnNameAcquired,
			OnNameLost,
			NULL,
			NULL);

	if (ownerId == 0) {
		LOG_LOGD("gdbus own failed!!");
		return false;
	}

	return true;
}


void
SyncService::InitializeDbus(void)
{
	LOG_LOGD("Dbus initialization starts");

	if (__initialize_dbus() == false) {
		LOG_LOGD("__initialize_dbus failed");
		exit(1);
	}

	g_hash_table = g_hash_table_new(g_str_hash, g_str_equal);
}


/*
 * DBus related initialization done
 */
SyncService::SyncService(void)
			: __pSyncManagerInstance(NULL)
{
	LOG_LOGD("Sync service initialization starts");

	InitializeDbus();
}


SyncService::~SyncService(void)
{
}

//}//_SyncManager
