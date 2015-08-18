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
#include <app.h>
#include <aul.h>
#include <app_manager.h>
#include <pkgmgr-info.h>
#include "SyncManager_SyncManager.h"
#include "sync-manager-stub.h"
#include "sync-ipc-marshal.h"
#include "sync-adapter-stub.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncAdapterAggregator.h"

static GDBusConnection* gdbusConnection = NULL;

/*namespace _SyncManager
{*/

#define SYNC_MANAGER_DBUS_PATH "/org/tizen/sync/manager"
#define SYNC_ADAPTER_DBUS_PATH "/org/tizen/sync/adapter/"
#define APP_CONTROL_OPERATION_START_SYNC "http://tizen.org/appcontrol/operation/start_sync"
#define APP_CONTROL_OPERATION_CANCEL_SYNC "http://tizen.org/appcontrol/operation/cancel_sync"

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
	SyncManager* pSyncManager = SyncManager::GetInstance();
	if (pSyncManager == NULL)
	{
		LOG_LOGD("Failed to initialize sync manager");
		return -1;
	}
	bool res = pSyncManager->Construct();
	if (!res)
	{
		LOG_LOGD("Sync manager construct failed");
	}

	return 0;
}

TizenSyncAdapter* adapter;


void
app_control_reply(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
	char *acc_id  = NULL;
	char *capability = NULL;
	char *app_id = NULL;
	char *sync_status = NULL;
	int	sync_result = 0;
	int accId = 0;
	account_h account = NULL;

	int ret = app_control_get_extra_data(reply, "sync_status", &sync_status);
	if (sync_status != NULL)
	{
		sync_result = atoi(sync_status);
	}

	ret = app_control_get_extra_data(request, "app_id", &app_id);
	SYNC_LOG_IF(ret != APP_CONTROL_ERROR_NONE, "app id cant be found");

	ret = app_control_get_extra_data(request, "acc_id", &acc_id);
	if (ret != APP_CONTROL_ERROR_KEY_NOT_FOUND)
	{
		app_control_get_extra_data(request, "capability", &capability);
		if( acc_id == NULL || capability == NULL)
		{
			LOG_LOGD("Invalid sync parameters");
			return;
		}
		if(acc_id != NULL)
		{
			accId = atoi(acc_id);
		}

		ret = account_create(&account);
		SYNC_LOG_IF(ret != ACCOUNT_ERROR_NONE, "account create failed");

		account_query_account_by_account_id(accId, &account);
		SYNC_LOG_IF(ret != ACCOUNT_ERROR_NONE, "account query failed");
	}

	SYNC_LOG_IF(app_id != NULL && acc_id != NULL && capability != NULL, "sync result %d received from %s %s %s", sync_result, app_id, acc_id, capability);

	SyncManager::GetInstance()->OnResultReceived((SyncStatus)sync_result, app_id, account, capability);
}


void
SyncService::TriggerStartSync(const char* appId, int accountId, bundle* pExtras, const char* pCapability)
{
	LOG_LOGD("appId %s", appId);

	app_control_h app_control;
	int ret = APP_CONTROL_ERROR_NONE;

	int isRunning = aul_app_is_running(appId);
	if (isRunning == 0)
	{
		LOG_LOGD("app is not running, launch the app and wait for signal");
		ret = app_control_create(&app_control);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

		ret = app_control_set_app_id(app_control, appId);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

		sa_app_id.clear();
		ret = app_control_send_launch_request(app_control, NULL, NULL);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control launch request failed %d", ret);

		// Wait till sync adapter is completely initialized
		while (sa_app_id.compare(appId));
		LOG_LOGD("Sync adapter initialized for this application");

		app_control_destroy(app_control);
	}

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	GVariant* pBundle = marshal_bundle(pExtras);

	tizen_sync_adapter_emit_start_sync(pSyncAdapter, accountId, pBundle, pCapability);
#if 0
	app_control_h app_control;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&app_control);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_set_app_id(app_control, appId);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_START_SYNC);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	char **bundleData = NULL;
	int items = bundle_export_to_argv(pExtras, &bundleData);

	ret = app_control_add_extra_data(app_control, "app_id", appId);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	if (accountId != -1)
	{
		char accId[50];
		sprintf(accId, "%d", accountId);
		LOGE("%s", accId);

		ret = app_control_add_extra_data(app_control, "acc_id", accId);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

		ret = app_control_add_extra_data(app_control, "capability", pCapability);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);
	}

	ret = app_control_add_extra_data_array(app_control, "bundle", (const char**)bundleData, items);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_send_launch_request(app_control, app_control_reply, NULL);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control launch request failed %d", ret);

	LOGE("Trigger start sync for %s account id %d capability %s", appId, accountId , pCapability);

	bundle_free_exported_argv(items, &bundleData);
	app_control_destroy(app_control);
#endif
}


void
SyncService::TriggerStopSync(const char* appId, account_h account, const char* pCapability)
{
	LOG_LOGD("Trigger stop sync %s", appId);
	int id = -1;

	if(account)
	{
		int ret = account_get_account_id(account, &id);
		if (ret != 0)
		{
			LOG_LOGD("Failed to get account id");
			return;
		}
		LOG_LOGD("account_id = %d", id);
	}

	TizenSyncAdapter* pSyncAdapter = (TizenSyncAdapter*) g_hash_table_lookup(g_hash_table, appId);
	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Failed to lookup syncadapter");
		return;
	}

	tizen_sync_adapter_emit_cancel_sync(pSyncAdapter, id, pCapability);

#if 0
	app_control_h app_control;
	int ret = APP_CONTROL_ERROR_NONE;

	ret = app_control_create(&app_control);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_set_app_id(app_control, appId);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_CANCEL_SYNC);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	if (account)
	{
		int acc_id;
		ret = account_get_account_id(account, &acc_id);
		LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "app control create failed %d", ret);

		char accId[50];
		sprintf(accId, "%d", acc_id);
		LOGE("%s", accId);

		ret = app_control_add_extra_data(app_control, "acc_id", accId);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

		ret = app_control_add_extra_data(app_control, "capability", pCapability);
		LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);
	}

	ret = app_control_add_extra_data(app_control, "app_id", appId);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control create failed %d", ret);

	ret = app_control_send_launch_request(app_control, app_control_reply, NULL);
	LOG_LOGE_VOID(ret == APP_CONTROL_ERROR_NONE, "app control launch request failed %d", ret);

	LOGE("Trigger cancel sync for %s", appId);

	app_control_destroy(app_control);
#endif
}


void
SyncService::RequestSync(const char* appId, int accountId, bundle* pExtras, const char* pCapability)
{
	SyncManager::GetInstance()->RequestSync(appId, accountId, pCapability, pExtras);
}


void
SyncService::CancelSync(const char* appId, account_h account, const char* pCapability)
{
	SyncManager::GetInstance()->CancelSync(appId, account, pCapability);
}


void
SyncService::AddPeriodicSyncJob(const char* appId, int accountId, bundle* pExtras, const char* pCapability, unsigned long pollFrequency)
{
	SyncManager::GetInstance()->AddPeriodicSyncJob(appId, accountId, pCapability ? pCapability : "", pExtras, pollFrequency);
}


void
SyncService::RemovePeriodicSyncJob(const char* appId, account_h account, bundle* pExtras, const char* pCapability)
{
	LOG_LOGD("SyncService::RemovePeriodicSyncJob() Starts");

	SyncManager::GetInstance()->RemovePeriodicSync(appId, account, pCapability, pExtras);

	LOG_LOGD("SyncService::RemovePeriodicSyncJob() Ends");
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
		LOG_LOGD("Gdbu error: Failed to get sender name");
		return -1;
	}

	GError *error = NULL;
	GDBusConnection* conn = g_dbus_method_invocation_get_connection(pMethodInvocation);
	GVariant *ret = g_dbus_connection_call_sync(conn, 	"org.freedesktop.DBus",
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
sync_adapter_handle_send_result(
		TizenSyncAdapter* pObject,
		GDBusMethodInvocation* pInvocation,
		gint sync_result,
		gint accountId,
		const gchar* pCapabilityArg)
{
	guint pid = get_caller_pid(pInvocation);
	char* pAppId = NULL;

	int ret = app_manager_get_app_id(pid, &pAppId);
	if(ret == APP_MANAGER_ERROR_NONE)
	{
		LOG_LOGD("Sync result: [%d] account id [%d] capability [%s]", sync_result, accountId, pCapabilityArg);

		account_h account = NULL;
		if (accountId != -1)
		{
			ret = account_create(&account);
			SYNC_LOG_IF(ret != ACCOUNT_ERROR_NONE, "account create failed");

			ret = account_query_account_by_account_id(accountId, &account);
			SYNC_LOG_IF(ret != ACCOUNT_ERROR_NONE, "account query failed for account id %d", accountId);
		}
		SyncManager::GetInstance()->OnResultReceived((SyncStatus)sync_result, pAppId, account, pCapabilityArg);
	}
	else
		LOG_LOGD("Get App Id fail %d", ret);

	tizen_sync_adapter_complete_send_result(pObject, pInvocation);

	aul_terminate_pid(pid);
	free(pAppId);
	return true;
}


gboolean
sync_adapter_handle_init_complete(
		TizenSyncAdapter* pObject,
		GDBusMethodInvocation* pInvocation)
{
	guint pid = get_caller_pid(pInvocation);
	char* pAppId = NULL;

	int ret = app_manager_get_app_id(pid, &pAppId);
	if(ret != APP_MANAGER_ERROR_NONE)
		LOG_LOGD("app_manager_get_app_id fail");

	sa_app_id.append(pAppId);

	LOG_LOGD("sync adapter client initialisation completed %s", pAppId);

	free(pAppId);
	tizen_sync_adapter_complete_init_complete(pObject, pInvocation);
	return true;
}


/*
* org.tizen.sync.manager interface methods
*/
gboolean
sync_manager_add_sync_job(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean account_less,
		gint accountId,
		GVariant* pBundleArg,
		const gchar* pCapabilityArg,
		const gchar* app_id)
{
	bundle* pBundle = umarshal_bundle(pBundleArg);

	if (account_less)
	{
		LOG_LOGD("On-Demand Sync appid [%s]", app_id);
		SyncService::GetInstance()->RequestSync(app_id, -1, pBundle, NULL);
	}
	else
	{
		LOG_LOGD("On-Demand Sync App ID [%s]: account id [%d] capability [%s] bundle [%s]", app_id, accountId, pCapabilityArg, pBundle?"Empty":"Non Empty");
		guint pid = get_caller_pid(pInvocation);

		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		SyncService::GetInstance()->RequestSync(app_id, accountId, pBundle, pCapabilityArg);
	}

	tizen_sync_manager_complete_add_sync_job(pObject, pInvocation);

	LOG_LOGD("sync service: Add sync job ends");

	return true;
}


gboolean
sync_manager_remove_sync_job(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean account_less,
		gint accountId,
		const gchar* pCapabilityArg,
		const gchar* app_id)
{
	if (account_less)
	{
		LOG_LOGD("Received account less remove sync request : %s", app_id);
		SyncService::GetInstance()->CancelSync(app_id, NULL, NULL);
	}
	else
	{
		account_h account = NULL;
		int ret = account_create(&account);
		LOG_LOGE_RESULT(ret == ACCOUNT_ERROR_NONE, "account create failed");

		int pid = SyncManager::GetInstance()->GetAccountPid(accountId);
		KNOX_CONTAINER_ZONE_ENTER(pid);
		ret =  account_query_account_by_account_id(accountId, &account);
		KNOX_CONTAINER_ZONE_EXIT();
		LOG_LOGE_RESULT(ret == ACCOUNT_ERROR_NONE, "account query failed");

		SyncService::GetInstance()->CancelSync(app_id, account, pCapabilityArg);
	}

	tizen_sync_manager_complete_remove_sync_job(pObject, pInvocation);
	LOG_LOGD("sync service: remove sync job ends");
	return true;
}


gboolean
sync_manager_add_periodic_sync_job(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean account_less,
		gint accountId,
		GVariant* pBundleArg,
		const gchar* pCapabilityArg,
		guint64 arg_poll_frequency,
		const gchar* app_id)
{
	bundle* pBundle = umarshal_bundle(pBundleArg);
	if (account_less)
	{
		LOG_LOGD("appId [%s] period [%d] ", app_id, arg_poll_frequency);
		SyncService::GetInstance()->AddPeriodicSyncJob(app_id, -1, pBundle, NULL, arg_poll_frequency);
	}
	else
	{
		LOG_LOGD("appId [%s] account id [%d] capability [%s] period [%d] bundle [%s]", app_id, accountId, pCapabilityArg, arg_poll_frequency, pBundle?"Empty":"Non Empty");
		guint pid = get_caller_pid(pInvocation);

		SyncManager::GetInstance()->AddRunningAccount(accountId, pid);
		SyncService::GetInstance()->AddPeriodicSyncJob(app_id, accountId, pBundle, pCapabilityArg, arg_poll_frequency);
	}
	tizen_sync_manager_complete_add_periodic_sync_job(pObject, pInvocation);

	LOG_LOGD("sync service: add periodic sync job ends");
	return true;
}


gboolean
sync_manager_remove_periodic_sync_job(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean account_less,
		gint accountId,
		const gchar* pCapabilityArg,
		const gchar* app_id)
{
	if (account_less)
	{
		LOG_LOGD("Received account less remove periodic sync request : %s", app_id);
		SyncService::GetInstance()->RemovePeriodicSyncJob(app_id, NULL, NULL, NULL);
	}
	else
	{
		LOG_LOGD("Received remove periodic sync request : %s", app_id);
		account_h account;
		int ret = account_create(&account);
		LOG_LOGE_RESULT(ret == ACCOUNT_ERROR_NONE, "account create failed");

		int pid = SyncManager::GetInstance()->GetAccountPid(accountId);
		KNOX_CONTAINER_ZONE_ENTER(pid);
		ret = account_query_account_by_account_id(accountId, &account);
		KNOX_CONTAINER_ZONE_EXIT();
		LOG_LOGE_RESULT(ret == ACCOUNT_ERROR_NONE, "account query failed for account id %d", accountId);

		SyncService::GetInstance()->RemovePeriodicSyncJob(app_id, account, NULL, pCapabilityArg);
	}

	tizen_sync_manager_complete_remove_periodic_sync_job(pObject, pInvocation);
	LOG_LOGD("sync service: remove periodic sync job ends");
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
sync_manager_add_sync_adapter(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean account_less_sa,
		const gchar* pCapability,
		const gchar* pCommandLine)
{
	LOG_LOGD("Add sync adapter request: account less = %d", account_less_sa);

	guint pid = get_caller_pid(pInvocation);

	string pkgId;
	char* pAppId = NULL;
	int ret = app_manager_get_app_id(pid, &pAppId);
	if(ret == APP_MANAGER_ERROR_NONE)
	{
		pkgId = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
	}
	else
	{
		pAppId = (char*) pCommandLine;
		pkgId = SyncManager::GetInstance()->GetPkgIdByCommandline(pCommandLine);
	}

	SyncAdapterAggregator* pAggregator = SyncManager::GetInstance()->GetSyncAdapterAggregator();
	if (pAggregator == NULL)
	{
		LOG_LOGD("sync adapter aggregator is NULL");
		tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);
		return true;
	}
	if (account_less_sa)
	{
		LOG_LOGD("app id %s svc-app id %s ", pkgId.c_str(), pAppId);
		pAggregator->AddSyncAdapter(pkgId.c_str(), pAppId, NULL);
	}
	else
	{
		LOG_LOGD("app id %s svc-app id %s capability %s", pkgId.c_str(), pAppId, pCapability);
		pAggregator->AddSyncAdapter(pkgId.c_str(), pAppId, pCapability);
	}

	char object_path[50];
	snprintf(object_path, 50, "%s%d", SYNC_ADAPTER_DBUS_PATH, pid);

	GError *error = NULL;
	GDBusInterfaceSkeleton* interface = NULL;
	TizenSyncAdapter* syncAdapterObj= tizen_sync_adapter_skeleton_new();
	if (syncAdapterObj == NULL)
	{
		LOG_LOGD("sync adapter object creation failed");
		tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);
		return true;
	}
	interface = G_DBUS_INTERFACE_SKELETON(syncAdapterObj);
	if (!g_dbus_interface_skeleton_export(interface, gdbusConnection, object_path, &error))
	{
		LOG_LOGD("export failed %s", error->message);
		tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);
		return true;
	}

	g_signal_connect(syncAdapterObj, "handle-send-result", G_CALLBACK(sync_adapter_handle_send_result), NULL);
	g_signal_connect(syncAdapterObj, "handle-init-complete", G_CALLBACK(sync_adapter_handle_init_complete), NULL);

	g_hash_table_insert(g_hash_table, pAppId, syncAdapterObj);

	LOG_LOGD("Sync adapter object path %s", object_path);

	tizen_sync_manager_complete_add_sync_adapter(pObject, pInvocation);

	return true;
}


gboolean
sync_manager_set_sync_status(
		TizenSyncManager* pObject,
		GDBusMethodInvocation* pInvocation,
		gboolean sync_enable)
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
	g_signal_connect(sync_ipc_obj, "handle-add-sync-job", G_CALLBACK(sync_manager_add_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-sync-job", G_CALLBACK(sync_manager_remove_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-periodic-sync-job", G_CALLBACK(sync_manager_add_periodic_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-remove-periodic-sync-job", G_CALLBACK(sync_manager_remove_periodic_sync_job), NULL);
	g_signal_connect(sync_ipc_obj, "handle-add-sync-adapter", G_CALLBACK(sync_manager_add_sync_adapter), NULL);
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
