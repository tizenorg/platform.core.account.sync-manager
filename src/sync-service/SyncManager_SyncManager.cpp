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
 * @brief	This is the implementation file for the SyncManager class.
 */

#include <sys/time.h>
#include <sys/stat.h>
#include <map>
#include <set>
#include <climits>
#include <stdlib.h>
#include <vconf.h>
#include <alarm.h>
#include <glib.h>
#include <aul.h>
#include <pkgmgr-info.h>
#include <tzplatform_config.h>
#include "sync-error.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncAdapterAggregator.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_PeriodicSyncJob.h"
#include "sync_manager.h"


/*namespace _SyncManager
{*/

#define VCONF_HOME_SCREEN  "db/setting/homescreen/package_name"
#define VCONF_LOCK_SCREEN  "file/private/lockscreen/pkgname"

#define SYNC_DATA_DIR tzplatform_mkpath(TZ_USER_DATA, "/sync-manager")

int DELAY_RETRY_SYNC_IN_PROGRESS_IN_SECONDS = 10;


void
SyncManager::SetSyncSetting(bool enable)
{
	bool wasSuspended = (__isSyncPermitted == false);
	__isSyncPermitted = enable;
	if (wasSuspended && __isSyncPermitted)
	{
		SendSyncCheckAlarmMessage();
	}
}


bool
SyncManager::GetSyncSetting()
{
	return __isSyncPermitted;
}


int
SyncManager::RequestSync(string appId, int accountId, const char* capability, bundle* pExtras)
{
	if (accountId == -1)
	{
		LOG_LOGD("Schedule account less sync");
		ScheduleAccountLessSync(appId, REASON_USER_INITIATED, pExtras, 0, 0, false);
	}
	else
	{
		LOG_LOGD("Schedule sync for account ID %d and capability %s", accountId, capability);
		ScheduleSync(appId, accountId, capability, REASON_USER_INITIATED, pExtras, 0, 0, false);
	}

	return SYNC_ERROR_NONE;
}


int
SyncManager::CancelSync(string appId, account_h account, const char* capability)
{
	LOG_LOGD("SyncManager::CancelSync Starts");

	ClearScheduledSyncJobs(appId, account, (capability != NULL)? capability : "");
	CancelActiveSyncJob(appId, account, (capability != NULL)? capability : "");

	LOG_LOGD("SyncManager::CancelSync Ends");
	return SYNC_ERROR_NONE;
}


int
SyncManager::AddPeriodicSyncJob(string appId, int accountId, const char* capability, bundle* pExtras, long period)
{
	if (period < 300)
	{
		LOG_LOGD("Requested period %d, rounding up to 300 sec", period);
		period = 300;
	}

	long defaultFlexTime = __pSyncRepositoryEngine->CalculateDefaultFlexTime(period);

	if (accountId == -1)
	{
		LOG_LOGD("Adding account less periodic sync for app [%s] period [%ld s]", appId.c_str(), period);

		char* pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(appId.c_str());
		if (pSyncAdapter == NULL)
		{
			LOG_LOGD("Sync adapter couldn't' be found for %s", appId.c_str());
			return SYNC_ERROR_SYSTEM;
		}

		PeriodicSyncJob* pRequestedJob = new (std::nothrow) PeriodicSyncJob(NULL, "", pExtras, period, defaultFlexTime);
		if (pRequestedJob == NULL)
		{
			LOG_LOGD("Failed to construct PeriodicSyncJob");
			return SYNC_ERROR_OUT_OF_MEMORY;
		}

		__pSyncRepositoryEngine->AddPeriodicSyncJob(pSyncAdapter, pRequestedJob, true);
	}
	else
	{
		LOG_LOGD("Adding periodic sync for [%s] capability [%s] period[%ld s]", appId.c_str(), capability, period);

		int pid = SyncManager::GetInstance()->GetAccountPid(accountId);

		account_h account = NULL;
		int ret = account_create(&account);
		SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, ret, "account access failed");

		KNOX_CONTAINER_ZONE_ENTER(pid);
		ret =  account_query_account_by_account_id(accountId, &account);
		KNOX_CONTAINER_ZONE_EXIT();

		SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, ret, "account query failed");

		account_sync_state_e syncSupport;
		ret = account_get_sync_support(account,  &syncSupport);
		SYNC_LOGE_RET_RES(ret == ACCOUNT_ERROR_NONE, ret, "account access failed");
		if (syncSupport == ACCOUNT_SYNC_INVALID || syncSupport == ACCOUNT_SYNC_NOT_SUPPORT)
		{
			LOG_LOGD("The account does not support sync");
			return SYNC_ERROR_INVALID_OPERATION;
		}

		char* pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(account, capability);
		if (pSyncAdapter == NULL)
		{
			return SYNC_ERROR_SYSTEM;
		}
		LOG_LOGD("Found sync adapter [%s]", pSyncAdapter);

		PeriodicSyncJob* pRequestedJob = new (std::nothrow) PeriodicSyncJob(account, capability, pExtras, period, defaultFlexTime);
		if (pRequestedJob == NULL)
		{
			LOG_LOGD("Failed to construct PeriodicSyncJob");
			return SYNC_ERROR_OUT_OF_MEMORY;
		}

		__pSyncRepositoryEngine->AddPeriodicSyncJob(pSyncAdapter, pRequestedJob, false);
	}

	return SYNC_ERROR_NONE;
}


int
SyncManager::RemovePeriodicSync(string appId, account_h account, const char* capability, bundle* pExtras)
{
	LOG_LOGD("SyncManager::removePeriodicSync Starts");

	if (appId.empty())
	{
		LOG_LOGD("Invalid parameter");
		return SYNC_ERROR_INVALID_PARAMETER;
	}
	//TODO check to write sync settings

	PeriodicSyncJob* pJobToRemove = new (std::nothrow) PeriodicSyncJob(account, capability ? capability : "", pExtras, 0, 0);
	if (pJobToRemove == NULL)
	{
		LOG_ERRORD("Failed to construct PeriodicSyncJob.");
		return SYNC_ERROR_OUT_OF_MEMORY;
	}

	__pSyncRepositoryEngine->RemovePeriodicSyncJob(appId, pJobToRemove);

	delete pJobToRemove;
	pJobToRemove = NULL;

	int ret = CancelSync(appId, account, capability);
	if (ret != SYNC_ERROR_NONE)
	{
		LOG_LOGD("Failed to cancel sync while removing periodic sync");
		return ret;
	}

	LOG_LOGD("SyncManager::removePeriodicSync Ends");
	return SYNC_ERROR_NONE;
}


SyncJobQueue*
SyncManager::GetSyncJobQueue(void) const
{
	return __pSyncJobQueue;
}


int
SyncManager::AddToSyncQueue(SyncJob* pJob)
{
	//No need to add mutex here, will be called during startup only
	return __pSyncJobQueue->AddSyncJob(*pJob);
}


//TODO: uncomment below lines for running accounts logic
void
SyncManager::AddRunningAccount(int account_id, int pid)
{
	__runningAccounts.insert(make_pair(account_id, pid));
	//__runningAccounts.push_back(account_id);
}


int
SyncManager::GetAccountPid(int account_id)
{
	map<int, int>::iterator it = __runningAccounts.find(account_id);
	if (it != __runningAccounts.end())
	{
		return it->second;
	}
	LOG_LOGD("Account id cant be found");
	return -1;
}


bool accountCb(account_h account, void* pUserData)
{
	int account_id = -1;
	int ret = account_get_account_id(account, &account_id);
	if (ret == ACCOUNT_ERROR_NONE)
	{
		SyncManager* pSyncManager = (SyncManager*)pUserData;
		pSyncManager->AddRunningAccount(account_id, 0);
	}
	return true;
}


void
SyncManager::UpdateRunningAccounts(void)
{
#if defined(_SEC_FEATURE_CONTAINER_ENABLE)
	return;
#endif
	__runningAccounts.clear();
	if (account_foreach_account_from_db(accountCb, this) < 0)
	{
		LOG_LOGD("UpdateRunningAccounts: Can not fetch account from db");
	}

	//TODO check additional android logic
}


bool OnAccountUpdated(const char* pEventType, int acountId, void* pUserData)
{
	//TODO: will go in enhancements
	SyncManager* pSyncManager = (SyncManager*)pUserData;
	pSyncManager->UpdateRunningAccounts();

	//pSyncManager->ScheduleSync(NULL, NULL, REASON_ACCOUNT_UPDATED, NULL, 0, 0, false);

	return true;
}


void
SyncManager::OnDNetStatusChanged(bool connected)
{
	LOG_LOGD("Data network change detected %d", connected);

	bool wasConnected = __isSimDataConnectionPresent;
	__isSimDataConnectionPresent = connected;
	if (__isSimDataConnectionPresent)
	{
		if (!wasConnected)
		{
			LOG_LOGD("Reconnection detected: clearing all backoffs");
			pthread_mutex_lock(&__syncJobQueueMutex);
			__pSyncRepositoryEngine->RemoveAllBackoffValuesLocked(__pSyncJobQueue);
			pthread_mutex_unlock(&__syncJobQueueMutex);
		}
		SendSyncCheckAlarmMessage();
	}
}


void
SyncManager::OnWifiStatusChanged(bool connected)
{
	LOG_LOGD("Wifi network change detected %d", connected);

	bool wasConnected = __isWifiConnectionPresent;
	__isWifiConnectionPresent = connected;
	if (__isWifiConnectionPresent)
	{
		if (!wasConnected)
		{
			LOG_LOGD("Reconnection detected: clearing all backoffs");
			pthread_mutex_lock(&__syncJobQueueMutex);
			__pSyncRepositoryEngine->RemoveAllBackoffValuesLocked(__pSyncJobQueue);
			pthread_mutex_unlock(&__syncJobQueueMutex);
		}
		SendSyncCheckAlarmMessage();
	}
}


void
SyncManager::OnBluetoothStatusChanged(bool connected)
{
	LOG_LOGD("Bluetooth status %d", connected);
}


void
SyncManager::OnStorageStatusChanged(int value)
{
	LOG_LOGD("Storage status changed %d", value);
	switch (value)
	{
	case LOW_MEMORY_NORMAL:
		__isStorageLow = false;
		break;
	case LOW_MEMORY_SOFT_WARNING:
		__isStorageLow = true;
		break;
	case LOW_MEMORY_HARD_WARNING:
		__isStorageLow = true;
		break;
	}
}

void
SyncManager::OnUPSModeChanged(bool enable)
{
	__isUPSModeEnabled = enable;
}


void
SyncManager::OnBatteryStatusChanged(int value)
{
	LOG_LOGD("SyncManager::OnBatteryStatusChanged Starts");

	switch (value)
	{
	case BAT_POWER_OFF:
		break;
	case BAT_CRITICAL_LOW:
		break;
	case BAT_WARNING_LOW:
		break;
	case BAT_NORMAL:
		break;
	case BAT_REAL_POWER_OFF:
		break;
	case BAT_FULL:
		break;
	}

	LOG_LOGD("SyncManager::OnBatteryStatusChanged Ends");
}


void SyncManager::OnCalendarDataChanged(int value)
{
	LOG_LOGD("SyncManager::OnCalendarDataChanged() Starts");

	if (value == CALENDAR_BOOK_CHANGED) {
		LOG_LOGD("Calendar Book Data Changed");
	} else if (value == CALENDAR_EVENT_CHANGED) {
		LOG_LOGD("Calendar Event Data Changed");
	} else if (value == CALENDAR_TODO_CHANGED) {
		LOG_LOGD("Calendar Todo Data Changed");
	}

	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncRepositoryEngine->RemoveAllBackoffValuesLocked(__pSyncJobQueue);
	pthread_mutex_unlock(&__syncJobQueueMutex);

	bundle *extra = bundle_create();
	bundle_add(extra, "SYNC_OPTION_UPLOAD", "true");

	ScheduleSync("org.tizen.calendar", -1, ACCOUNT_SUPPORTS_CAPABILITY_CALENDAR, REASON_DEVICE_DATA_CHANGED, extra, 0, 0, false);

	SendSyncCheckAlarmMessage();

	bundle_free(extra);
	LOG_LOGD("SyncManager::OnCalendarDataChanged() Ends");
}


void SyncManager::OnContactsDataChanged(int value)
{
	LOG_LOGD("SyncManager::OnContactsDataChanged() Starts");

	if (value == CONTACTS_DATA_CHANGED) {
		LOG_LOGD("Contacts Data Changed");
	}

	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncRepositoryEngine->RemoveAllBackoffValuesLocked(__pSyncJobQueue);
	pthread_mutex_unlock(&__syncJobQueueMutex);

	bundle *extra = bundle_create();
	bundle_add(extra, "SYNC_OPTION_UPLOAD", "true");

	ScheduleSync("org.tizen.contacts", -1, ACCOUNT_SUPPORTS_CAPABILITY_CONTACT, REASON_DEVICE_DATA_CHANGED, extra, 0, 0, false);

	SendSyncCheckAlarmMessage();

	bundle_free(extra);
	LOG_LOGD("SyncManager::OnContactsDataChanged() Ends");
}


static int OnPackageUninstalled(unsigned int userId, int reqId, const char* pPkgType, const char* pPkgId, const char* pKey,
									const char* pVal, const void* pMsg, void* pData)
{
	LOG_LOGD("OnPackageUninstalled [type %s] type [pkdId:%s]", pPkgType, pPkgId);
	pthread_mutex_t* pSyncJobQueueMutex = static_cast< pthread_mutex_t* > (pData);
	if (!strcmp("end", pKey) && !strcmp("ok", pVal))
	{
		SyncManager::GetInstance()->GetSyncAdapterAggregator()->HandlePackageUninstalled(pPkgId);

		RepositoryEngine::GetInstance()->CleanUp(pPkgId);

		if (SyncManager::GetInstance()->GetSyncJobQueue())
		{
			pthread_mutex_lock(pSyncJobQueueMutex);
			SyncManager::GetInstance()->GetSyncJobQueue()->RemoveSyncJobsForApp(pPkgId);
			pthread_mutex_unlock(pSyncJobQueueMutex);
		}
	}

	return 0;
}


string
SyncManager::GetPkgIdByAppId(const char* pAppId)
{
	pkgmgrinfo_appinfo_h handle;
	string pkgId;

	int result = pkgmgrinfo_appinfo_get_appinfo(pAppId, &handle);
	if (result == PMINFO_R_OK)
	{
		char* pPkgId = NULL;

		result = pkgmgrinfo_appinfo_get_pkgid(handle, &pPkgId);
		if (result == PMINFO_R_OK)
		{
			pkgId.append(pPkgId);
		}
		else
		{
			LOG_LOGD("Failed to get Pkg ID from App Id [%s]", pAppId);
		}
		pkgmgrinfo_appinfo_destroy_appinfo(handle);
	}
	else
	{
		LOG_LOGD("Failed to get pkgmgr AppInfoHandle from App Id [%s]", pAppId);
	}
	return pkgId;
}


string
SyncManager::GetPkgIdByCommandline(const char* pCommandLine)
{
	string pkgId;
	char cmd[100];
	memset(cmd, 0x00, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "rpm -qf %s --queryformat '%{name}\\t'", pCommandLine);

	FILE* pipe = popen(cmd, "r");
	if (!pipe)
	{
		LOG_LOGD("Failed to open pipe.");
		return pkgId;
	}

	char *buffer = NULL;
	size_t len = 0;
	while (!feof(pipe))
	{
		if (getdelim(&buffer, &len, '\t', pipe) != -1)
		{
			pkgId = buffer;
		}
	}
	pclose(pipe);
	free(buffer);

	return pkgId;
}



void
SyncManager::RegisterForNetworkChange(void)
{
	if (__pNetworkChangeListener)
	{
		if(!__pNetworkChangeListener->RegisterNetworkChangeListener())
		{
			LOG_LOGD("Network listener : Success");
		}
		else
		{
			LOG_LOGD("Network listener : Failed");
		}
	}
}


int
SyncManager::DeRegisterForNetworkChange(void)
{
	if (__pNetworkChangeListener)
	{
		return(__pNetworkChangeListener->DeRegisterNetworkChangeListener());
	}
	return -1;
}

void OnUPSModeChangedCb(keynode_t* pKey, void* pData)
{
	int value = vconf_keynode_get_int(pKey);
	bool enabled = (value == SETTING_PSMODE_EMERGENCY);
	LOG_LOGD("UPS mode status %d , value %d",enabled, value);

	SyncManager::GetInstance()->OnUPSModeChanged(enabled);
}


void
SyncManager::RegisterForUPSModeChange(void)
{
	if(vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE, OnUPSModeChangedCb, NULL) == 0)
	{
		LOG_LOGD("UPS mode listener : Success");
	}
	else
	{
		LOG_LOGD("UPS mode listener : Failed");
	}
}


int
SyncManager::DeRegisterForUPSModeChange(void)
{
	LOG_LOGD("De Registering UPS mode listener");

	return vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE, OnUPSModeChangedCb);
}


void
SyncManager::RegisterForStorageChange(void)
{
	if (__pStorageListener)
	{
		if (__pStorageListener->RegisterStorageChangeListener() == 0)
		{
			LOG_LOGD("Storage listener : Success");
		}
		else
		{
			LOG_LOGD("Storage listener : Failed");
		}
	}
}


int
SyncManager::DeRegisterForStorageChange(void)
{
	if (__pStorageListener)
	{
		return(__pStorageListener->DeRegisterStorageChangeListener());
	}
	return -1;
}


void
SyncManager::RegisterForBatteryStatus(void)
{
	if (__pBatteryStatusListener)
	{
		if(__pBatteryStatusListener->RegisterBatteryStatusListener() == 0)
		{
			LOG_LOGD("Battery listener : Success");
		}
		else
		{
			LOG_LOGD("Battery listener : Failed");
		}
	}
}


int
SyncManager::DeRegisterForBatteryStatus(void)
{
	if (__pBatteryStatusListener)
	{
		return(__pBatteryStatusListener->DeRegisterBatteryStatusListener());
	}
	return -1;
}


void SyncManager::RegisterForDataChange(void)
{
	if (__pDataChangeListener)
	{
		if(!__pDataChangeListener->RegisterDataChangeListener())
		{
			LOG_LOGD("Data listener : Success");
		}
		else
		{
			LOG_LOGD("Data listener : Failed");
		}
	}
}


int SyncManager::DeRegisterForDataChange(void)
{
	if (__pDataChangeListener)
	{
		return (__pDataChangeListener->DeRegisterDataChangeListener());
	}

	return -1;
}


int
SyncManager::SetPkgMgrClientStatusChangedListener(void)
{
	int eventType = PKGMGR_CLIENT_STATUS_UNINSTALL;

	if (pkgmgr_client_set_status_type(__pPkgmgrClient, eventType) != PKGMGR_R_OK)
	{
		LOG_LOGD("pkgmgr_client_set_status_type failed.");
		pkgmgr_client_free(__pPkgmgrClient);
		__pPkgmgrClient = NULL;
		return -1;
	}

	if (pkgmgr_client_listen_status(__pPkgmgrClient, OnPackageUninstalled, &__syncJobQueueMutex) < 0)
	{
		LOG_LOGD("pkgmgr_client_listen_status failed.");
		pkgmgr_client_free(__pPkgmgrClient);
		__pPkgmgrClient = NULL;
		return -1;
	}

	return 0;
}


RepositoryEngine*
SyncManager::GetSyncRepositoryEngine(void)
{
	return __pSyncRepositoryEngine;
}


void
SyncManager::ClearScheduledSyncJobs(string appId, account_h account, string capability)
{
	pthread_mutex_lock(&__syncJobQueueMutex);
	char* pSyncAdapter = NULL;
	if (account != NULL)
	{
		pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(account, capability);
		GetSyncRepositoryEngine()->SetBackoffValue(appId, account, capability, GetSyncRepositoryEngine()->NOT_IN_BACKOFF_MODE,
											   GetSyncRepositoryEngine()->NOT_IN_BACKOFF_MODE);
	}
	else
	{
		pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(appId.c_str());
	}

	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Sync adapter not found for give job");
		return;
	}

	__pSyncJobQueue->RemoveSyncJob(pSyncAdapter, account, capability);

	pthread_mutex_unlock(&__syncJobQueueMutex);
}


void
SyncManager::CancelActiveSyncJob(string appId, account_h account, string capability)
{
	string key;
	char* pSyncAdapter = NULL;

	if (account != NULL && !capability.empty())
	{
		 key = CurrentSyncJobQueue::ToKey(account, capability);
		 pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(account, capability);
	}
	else
	{
		key.append("id:").append(appId);
		pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(appId.c_str());
	}
	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Sync adapter not found for give job");
		return;
	}

	pthread_mutex_lock(&__currJobQueueMutex);
	CurrentSyncContext *pCurrSyncContext = __pCurrentSyncJobQueue->GetCurrJobfromKey(key);
	pthread_mutex_unlock(&__currJobQueueMutex);
	if (pCurrSyncContext != NULL)
	{
		g_source_remove(pCurrSyncContext->GetTimerId());
		SyncJob* pJob = new (std::nothrow) SyncJob(*(pCurrSyncContext->GetSyncJob()));
		CloseCurrentSyncContext(pCurrSyncContext);
		SendCancelSyncsMessage(pJob);
	}
	else
	{
		 SyncService::GetInstance()->TriggerStopSync(pSyncAdapter, account, capability.c_str());
	}

}


SyncManager::SyncManager(void)
	: __isStorageLow (false)
	, __isWifiConnectionPresent(false)
	, __isSimDataConnectionPresent(false)
	, __isUPSModeEnabled(false)
	, __isSyncPermitted(true)
	, __pNetworkChangeListener(NULL)
	, __pStorageListener(NULL)
	, __pBatteryStatusListener(NULL)
	, __pDataChangeListener(NULL)
	, __pSyncRepositoryEngine(NULL)
	, __pSyncJobQueue(NULL)
	, __pSyncJobDispatcher(NULL)
	, __pSyncAdapterAggregator(NULL)
	, __pCurrentSyncJobQueue(NULL)
	, __accountSubscriptionHandle(NULL)
{

}


bool
SyncManager::Construct(void)
{
	//interface=org.freedesktop.systemd1.Manager Signal=StartupFinished - bootcomplete dbus signal
	LOG_LOGD("Sync manager initialization begins");

	int storageState;
	int ret = vconf_get_int(VCONFKEY_SYSMAN_LOW_MEMORY, &storageState);
	LOG_LOGE_BOOL(ret == VCONF_OK, "vconf_get_int failed");
	__isStorageLow = (storageState == LOW_MEMORY_NORMAL) ? false : true;

	int upsMode;

	if (-1 == access (SYNC_DATA_DIR, F_OK)) {
		mkdir(SYNC_DATA_DIR, 755);
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &upsMode);
	LOG_LOGE_BOOL(ret == VCONF_OK, "vconf_get_int failed");
	__isUPSModeEnabled = (upsMode == SETTING_PSMODE_EMERGENCY) ? true : false;

	__pNetworkChangeListener = new (std::nothrow) NetworkChangeListener();
	LOG_LOGE_BOOL(__pNetworkChangeListener, "Failed to construct NetworkChangeListener");

	__pStorageListener = new (std::nothrow) StorageChangeListener();
	LOG_LOGE_BOOL(__pStorageListener, "Failed to construct StorageChangeListener");

	__pBatteryStatusListener = new (std::nothrow) BatteryStatusListener();
	LOG_LOGE_BOOL(__pBatteryStatusListener, "Failed to construct BatteryStatusListener");

	__pDataChangeListener = new (std::nothrow) DataChangeListener();
	LOG_LOGE_BOOL(__pDataChangeListener, "Failed to Construct DataChangeListener");

	__pSyncAdapterAggregator = new (std::nothrow) SyncAdapterAggregator();
	LOG_LOGE_BOOL(__pSyncAdapterAggregator, "Failed to construct SyncJobDispatcher");

	__pSyncRepositoryEngine = RepositoryEngine::GetInstance();
	LOG_LOGE_BOOL(__pSyncRepositoryEngine, "Failed to construct RepositoryEngine");

	__pSyncJobQueue = new (std::nothrow) SyncJobQueue(__pSyncRepositoryEngine);
	LOG_LOGE_BOOL(__pSyncJobQueue, "Failed to construct SyncJobQueue");

	__pCurrentSyncJobQueue = new (std::nothrow) CurrentSyncJobQueue();
	LOG_LOGE_BOOL(__pCurrentSyncJobQueue, "Failed to construct CurrentSyncJobQueue");

	__pSyncJobDispatcher = new (std::nothrow) SyncJobDispatcher();
	LOG_LOGE_BOOL(__pSyncJobDispatcher, "Failed to construct SyncJobDispatcher");

	__isWifiConnectionPresent = __pNetworkChangeListener->IsWifiConnected();
	__isSimDataConnectionPresent = __pNetworkChangeListener->IsDataConnectionPresent();

	LOG_LOGD("wifi %d, sim %d storage %d", __isWifiConnectionPresent, __isSimDataConnectionPresent, __isStorageLow);

	LOG_LOGD("Register device status listeners");
	RegisterForNetworkChange();
	RegisterForStorageChange();
	RegisterForBatteryStatus();
	RegisterForDataChange();
	RegisterForUPSModeChange();

	__randomOffsetInMillis = __pSyncRepositoryEngine->GetRandomOffsetInsec() * 1000;

	LOG_LOGE_BOOL(pthread_mutex_init(&__syncJobQueueMutex, NULL) == 0, "__syncJobQueueMutex init failed");
	LOG_LOGE_BOOL(pthread_mutex_init(&__currJobQueueMutex, NULL) == 0, "__currJobQueueMutex init failed");

	__pPkgmgrClient = pkgmgr_client_new(PC_LISTENING);
	LOG_LOGE_BOOL(__pPkgmgrClient != NULL, "__pPkgmgrClient is null");

	LOG_LOGE_BOOL(SetPkgMgrClientStatusChangedListener() == 0, "Failed to register for uninstall callback.");

	UpdateRunningAccounts();

	if (account_subscribe_create(&__accountSubscriptionHandle) < 0)
	{
		LOG_LOGD("Failed to create account subscription handle");
	}
	else if (account_subscribe_notification(__accountSubscriptionHandle, OnAccountUpdated, this) < 0)
	{
		LOG_LOGD("Failed to register callback for account updation");
	}
	else
		LOG_LOGD("Account connect failed");

	Initialize();

	return true;
}


SyncManager::~SyncManager(void)
{
	LOG_LOGD("SyncManager::~SyncManager() Starts");

	pthread_mutex_destroy(&__syncJobQueueMutex);
	pthread_mutex_destroy(&__currJobQueueMutex);

	DeRegisterForNetworkChange();
	DeRegisterForStorageChange();
	DeRegisterForBatteryStatus();
	DeRegisterForDataChange();
	DeRegisterForUPSModeChange();

	delete __pNetworkChangeListener;
	delete __pStorageListener;
	delete __pBatteryStatusListener;
	delete __pDataChangeListener;
	delete __pSyncRepositoryEngine;
	delete __pSyncJobQueue;
	delete __pSyncJobDispatcher;
	delete __pCurrentSyncJobQueue;
	delete __pSyncAdapterAggregator;

	if (__pPkgmgrClient)
	{
		pkgmgr_client_free(__pPkgmgrClient);
	}
	__pPkgmgrClient = NULL;

	//TODO: uncomment below lines for running accounts logic
	/*if (account_unsubscribe_notification(__accountSubscriptionHandle) < 0)
	{
		LOG_LOGD("SyncManager::SyncManager failed to deregister callback for account updation");
	}*/
	LOG_LOGD("SyncManager::~SyncManager() Ends");
}


bool
SyncManager::AreAccountsEqual(account_h account1, account_h account2)
{
	bool isEqual = false;
	int id1, id2;
	if (account_get_account_id(account1, &id1) < 0)
	{
		isEqual = false;
	}
	if (account_get_account_id(account2, &id2) < 0)
	{
		isEqual = false;
	}

	char* pName1;
	char* pName2;
	if (account_get_user_name(account1, &pName1) < 0)
	{
		isEqual = false;
	}
	if (account_get_user_name(account2, &pName2) < 0)
	{
		isEqual = false;
	}

	if (id1 == id2 && strcmp(pName1, pName2) == 0)
	{
		isEqual = true;
	}

	return isEqual;
}

void
SyncManager::IncreaseBackoffValue(SyncJob* pJob)
{
	LOG_LOGD("Increase backoff");

	if (pJob == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return;
	}

	time_t  now;
	time(&now);

	backOff* prevBackOff = __pSyncRepositoryEngine->GetBackoffN(pJob->account, pJob->capability);
	if (prevBackOff == NULL)
	{
		LOG_LOGD("No backoff exists");
		delete prevBackOff;
		prevBackOff = NULL;
		return;
	}
	long newDelayInMs = -1;
	if (now < prevBackOff->time)
	{
		delete prevBackOff;
		prevBackOff = NULL;
		return;
	}
	// Subsequent delays are the double of the previous delay
	newDelayInMs = prevBackOff->delay * 2;

	delete prevBackOff;
	prevBackOff = NULL;

	long backoff = now + newDelayInMs;

	__pSyncRepositoryEngine->SetBackoffValue(pJob->appId, pJob->account, pJob->capability, backoff, newDelayInMs);

	pJob->backoff = backoff;
	pJob->UpdateEffectiveRunTime();

	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncJobQueue->OnBackoffChanged(pJob->account, pJob->capability, backoff);
	pthread_mutex_unlock(&__syncJobQueueMutex);
}


bool
SyncManager::IsActiveAccount(vector<account_h> accounts, account_h account)
{
 return true;
 //TODO: uncomment while implementing __running accounts logic
 /*
	for (unsigned int i = 0; i < accounts.size(); i++)
	{
		if (AreAccountsEqual(accounts[i], account))
		{
			return true;
		}
	}

	return false;*/
}


long long
SyncManager::GetElapsedTime(void)
{
	long long elapsedTime = 0;
	float a = 0.0, b = 0.0;
	FILE *fp = fopen("/proc/uptime", "r");

	if (fscanf(fp, "%f %f", &a, &b))
	{
		elapsedTime = (long long)(a*1000.f);
	}

	fclose(fp);
	return elapsedTime;
}

SyncAdapterAggregator*
SyncManager::GetSyncAdapterAggregator()
{
	return __pSyncAdapterAggregator;
}

void
SyncManager::HandleShutdown(void)
{
	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncRepositoryEngine->SaveCurrentState();
	pthread_mutex_unlock(&__syncJobQueueMutex);
}


void
SyncManager::ClearBackoffValue(SyncJob* pJob)
{
	LOG_LOGD("Clear backoff");
	if (pJob == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return;
	}

	__pSyncRepositoryEngine->SetBackoffValue(pJob->appId, pJob->account, pJob->capability, BackOffMode::NOT_IN_BACKOFF_MODE, BackOffMode::NOT_IN_BACKOFF_MODE);

	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncJobQueue->OnBackoffChanged(pJob->account, pJob->capability, 0);
	pthread_mutex_unlock(&__syncJobQueueMutex);
}

int
SyncManager::GetSyncable(account_h account, string capability)
{
	int syncable = GetSyncRepositoryEngine()->GetSyncable(account, capability);

	return syncable;
	//TODO check android logic getIsSyncable, that checks for restricted user and
	// checks if the sync adapter has opted in or out
}


void
SyncManager::SendCancelSyncsMessage(SyncJob* pJob)
{
	LOG_LOGD("SyncManager::SendCancelSyncsMessage :sending MESSAGE_CANCEL");
	Message msg;
	msg.type = SYNC_CANCEL;
	msg.pSyncJob = pJob;
	FireEvent(__pSyncJobDispatcher, msg);
}


void
SyncManager::OnResultReceived(SyncStatus res, string appId, account_h account, const char* capability)
{
	LOG_LOGD("Sync result received from %s", appId.c_str());
	string key;
	if (account != NULL && capability != NULL)
	{
		 key = CurrentSyncJobQueue::ToKey(account, capability);
	}
	else
	{
		key.append("id:").append(appId);
	}

	LOG_LOGD("Close Sync context for key %s", key.c_str());

	pthread_mutex_lock(&__currJobQueueMutex);
	CurrentSyncContext *pCurrSyncContext = __pCurrentSyncJobQueue->GetCurrJobfromKey(key);
	pthread_mutex_unlock(&__currJobQueueMutex);
	if (pCurrSyncContext == NULL)
	{
		LOG_LOGD("Sync context cant be found for %s", key.c_str());
	}
	else
	{
		g_source_remove(pCurrSyncContext->GetTimerId());
		SyncJob* pJob = new (std::nothrow) SyncJob(*(pCurrSyncContext->GetSyncJob()));
		CloseCurrentSyncContext(pCurrSyncContext);
		SendSyncCompletedOrCancelledMessage(pJob, res);
	}
}

void
SyncManager::CloseCurrentSyncContext(CurrentSyncContext *activeSyncContext)
{
	if (activeSyncContext == NULL)
	{
		LOG_LOGD("Invalid Parameter");
		return;
	}
	pthread_mutex_lock(&(__currJobQueueMutex));
	__pCurrentSyncJobQueue->RemoveSyncJobFromCurrentSyncQueue(activeSyncContext);
	pthread_mutex_unlock(&(__currJobQueueMutex));
}


void
SyncManager::SendSyncCompletedOrCancelledMessage(SyncJob *pJob, int result)
{
	LOG_LOGD("SyncManager::SendSyncCompletedOrCancelledMessage");
	Message msg;
	msg.res = (SyncStatus)result;
	msg.pSyncJob = pJob;
	msg.type = SYNC_FINISHED;
	FireEvent(__pSyncJobDispatcher, msg);
}


void
SyncManager::AlertForChange()
{
	SendSyncCheckAlarmMessage();
}

void
SyncManager::SendSyncAlarmMessage()
{
	LOG_LOGD("Fire SYNC_ALARM");
	Message msg;
	msg.type = SYNC_ALARM;
	FireEvent(__pSyncJobDispatcher, msg);
}

void
SyncManager::SendSyncCheckAlarmMessage()
{
	LOG_LOGD("Fire SYNC_CHECK_ALARM ");
	Message msg;
	msg.type = SYNC_CHECK_ALARM;
	//TO DO: Implement code to remove all the pending messages from queue before firing a new one
	FireEvent(__pSyncJobDispatcher, msg);
}

/*
 *
 */
bool
SyncManager::GetBundleVal(const char* pVal)
{
	if (pVal == NULL)
	{
		return false;
	}
	else return strcmp(pVal, "true")? true: false;
}


bool get_capability_all_cb(const char* capability_type, account_capability_state_e capability_state, void *user_data)
{
	set<string>* pSsncableCapabilities = (set<string>*)user_data;

	if (capability_state == ACCOUNT_CAPABILITY_ENABLED)
	{
		pSsncableCapabilities->insert(capability_type);
	}
	return true;
}


void
SyncManager::ScheduleAccountLessSync(string appId, int reason, bundle* pExtras,
									 long long flexTimeInMillis, long long runTimeInMillis, bool onlyForUnknownSyncableState)
{
	if (!pExtras)
	{
		pExtras = bundle_create();
	}

	const char* pVal = bundle_get_val(pExtras, SYNC_OPTION_EXPEDITED);
	bool isExpedited = GetBundleVal(pVal);
	pVal = NULL;
	if (isExpedited)
	{
		//Schedule at the front of the queue
		runTimeInMillis = -1;
	}

	pVal = bundle_get_val(pExtras, "SYNC_OPTION_UPLOAD");
	bool toUploadOnly = GetBundleVal(pVal);
	pVal = NULL;

	bundle_add(pExtras, "SYNC_OPTION_IGNORE_BACKOFF", "true");
	bundle_add(pExtras, "SYNC_OPTION_IGNORE_SETTINGS", "true");
	pVal = NULL;

	int syncSource;
	if (toUploadOnly)
	{
		syncSource = SOURCE_LOCAL;
	}
	else
	{
		syncSource = SOURCE_SERVER;
	}

	long long elapsedTime = GetElapsedTime();
	LOG_LOGD("ScheduleManualSyncJobs running right now, at %lld", elapsedTime);

	char* pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(appId.c_str());
	if (pSyncAdapter == NULL)
	{
		LOG_LOGD("Sync adapter cant be found for %s", appId.c_str());
		return;
	}
	SyncJob* pJob = new (std::nothrow) SyncJob(pSyncAdapter, NULL, "",
										pExtras, (SyncReason)reason, (SyncSource)syncSource,
										runTimeInMillis, flexTimeInMillis, 0,
										0,
										false);
	if (pJob == NULL)
	{
		LOG_LOGD("Failed to construct SyncJob");
		return;
	}

	long long elapsedTime1 = GetElapsedTime();
	LOG_LOGD("ScheduleManualSyncJobs, going to run right now at %lld", elapsedTime1);
	ScheduleSyncJob(pJob);
	delete pJob;
	pJob = NULL;
}


void
SyncManager::ScheduleSync(string appId, int accountId, string capability, int reason, bundle* pExtras,
		long long flexTimeInMillis, long long runTimeInMillis, bool onlyForUnknownSyncableState)
{
	if (!pExtras)
	{
		pExtras = bundle_create();
	}

	const char* pVal = bundle_get_val(pExtras, SYNC_OPTION_EXPEDITED);
	bool isExpedited = GetBundleVal(pVal);
	pVal = NULL;
	if (isExpedited)
	{
		//Schedule at the front of the queue
		runTimeInMillis = -1;
	}

	map<int, int> accounts;
	if (accountId != -1)
	{
		int pid = GetAccountPid(accountId);
		accounts.insert(make_pair(accountId, pid));
	}
	else
	{
		accounts = __runningAccounts;
		LOG_LOGE_VOID(accounts.size() != 0, "Accounts are not available yet, returning");
	}

	LOG_LOGD("Running accounts size %d", accounts.size());

	pVal = bundle_get_val(pExtras, "SYNC_OPTION_UPLOAD");
	bool toUploadOnly = GetBundleVal(pVal);
	pVal = NULL;

	bundle_add(pExtras, "SYNC_OPTION_IGNORE_BACKOFF", "true");
	bundle_add(pExtras, "SYNC_OPTION_IGNORE_SETTINGS", "true");

	pVal = bundle_get_val(pExtras, "SYNC_OPTION_IGNORE_SETTINGS");
	bool ignoreSettings = GetBundleVal(pVal);
	pVal = NULL;

	int syncSource;
	if (toUploadOnly)
	{
		syncSource = SOURCE_LOCAL;
	}
	else if (capability.empty())
	{
		syncSource = SOURCE_POLL;
	}
	else
	{
		syncSource = SOURCE_SERVER;
	}

	map<int, int>::iterator it;
	for (it = accounts.begin(); it != accounts.end(); it++)
	{
		int account_id = it->first;
		int pid = it->second;

		account_h currentAccount = NULL;
		int ret = account_create(&currentAccount);
		LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account access failed");

		KNOX_CONTAINER_ZONE_ENTER(pid);
		ret =  account_query_account_by_account_id(account_id, &currentAccount);
		KNOX_CONTAINER_ZONE_EXIT();
		LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account query failed");

		account_sync_state_e syncSupport;
		ret = account_get_sync_support(currentAccount,  &syncSupport);
		LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account access failed");

		if (syncSupport == ACCOUNT_SYNC_INVALID || syncSupport == ACCOUNT_SYNC_NOT_SUPPORT)
		{
			LOG_LOGD("The account does not support sync");
			continue;
		}

		/*
		 * If capability parameter is not null, check if it is syncable
		 * If syncable, clear the set and add just this capability.
		 * else, just clear the set
		 */
		 set<string> syncableCapabilities;
		if (capability.empty())
		{
			ret = account_get_capability_all(currentAccount, get_capability_all_cb, (void *)&syncableCapabilities);
			LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account get capability all failed");

			LOG_LOGD("Sync jobs will be added for all sync enabled capabilities of the account");
		}
		else
		{
			syncableCapabilities.insert(capability);
		}
		set<string>::iterator it;

		for (it = syncableCapabilities.begin(); it != syncableCapabilities.end(); it++)
		{
			int ret = account_get_sync_support(currentAccount,  &syncSupport);
			LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account access failed");

			char* user_name;
			ret = account_get_user_name(currentAccount, &user_name);
			LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "account access failed");

			string currentCapability = *it;
			LOG_LOGD("Current Account Info: Id %d, user_name %s, total account size %d", account_id, user_name, accounts.size());
			LOG_LOGD("Current capability %s", currentCapability.c_str());

			int syncable = GetSyncable(currentAccount, currentCapability);
			if (syncable == 0)
			{
				continue;
			}
			char* pSyncAdapter = __pSyncAdapterAggregator->GetSyncAdapter(currentAccount, currentCapability);
			if (pSyncAdapter == NULL)
			{
				LOG_LOGD("Sync disabled for account capability pair");
				continue;
			}

			bool allowParallelSyncs = false;//TODO replace true with syncAdapterInfo.type.allowParallelSyncs();
			bool isAlwaysSyncable = true;//TODO replace true with syncAdapterInfo.type.isAlwaysSyncable();

			if (syncable < 0 && isAlwaysSyncable)
			{
				GetSyncRepositoryEngine()->SetSyncable(pSyncAdapter, currentAccount, currentCapability, 1);
				syncable = 1;
			}
			if (onlyForUnknownSyncableState && syncable >= 0)
			{
				continue;
			}

			//TODO if (!syncAdapterInfo.type.supportsUploading() && uploadOnly) {continue;}
			bool bootCompleted = false; //TODO remove this line and populate bootcompleted using dbus signal handler

			bool isSyncAllowed = (syncable < 0) //Unknown syncable state
								|| ignoreSettings
								|| (!bootCompleted
									&& GetSyncRepositoryEngine()->GetSyncAutomatically(currentAccount, currentCapability)
									/*TODO && GetSyncRepositoryEngine()->getMasterSyncAutomatically(currentAccount)*/);
			if (!isSyncAllowed)
			{
				LOG_LOGD("sync not allowed, capability %s", currentCapability.c_str());
				continue;
			}

			backOff* pBackOff = GetSyncRepositoryEngine()->GetBackoffN(currentAccount, currentCapability);
			long delayUntil = GetSyncRepositoryEngine()->GetDelayUntilTime(currentAccount, currentCapability);
			long backOffTime = pBackOff ? pBackOff->time : 0;
			delete pBackOff;
			pBackOff = NULL;

			if (!onlyForUnknownSyncableState)
			{
				SyncJob* pJob = new (std::nothrow) SyncJob(pSyncAdapter, currentAccount,
													currentCapability,
													pExtras, (SyncReason)reason, (SyncSource)syncSource,
													runTimeInMillis, flexTimeInMillis, backOffTime,
													delayUntil,
													allowParallelSyncs);
				if (pJob == NULL)
				{
					LOG_LOGD("Failed to construct SyncJob");
					continue;
				}
				ScheduleSyncJob(pJob);
				delete pJob;
				pJob = NULL;
			}
		}
	}

	if (!pExtras)
	{
		bundle_free(pExtras);
	}
}


void
SyncManager::ScheduleSyncJob(SyncJob* pJob, bool fireCheckAlarm)
{
	int err;
	pthread_mutex_lock(&__syncJobQueueMutex);
	err = __pSyncJobQueue->AddSyncJob(*pJob);
	pthread_mutex_unlock(&__syncJobQueueMutex);

	if (err == SYNC_ERROR_NONE)
	{
		if(fireCheckAlarm)
		{
			LOG_LOGD("Added sync job [%s] to Main queue, Intiating dispatch sequence", pJob->key.c_str());
			SendSyncCheckAlarmMessage();
		}
	}
	else if (err == SYNC_ERROR_ALREADY_IN_PROGRESS)
	{
		LOG_LOGD("Duplicate sync job [%s], No need to enqueue", pJob->key.c_str());
	}
	else
	{
		LOG_LOGD("Failed to add into sync job list");
	}
}

void
SyncManager::TryToRescheduleJob(SyncStatus syncResult, SyncJob* pJob)
{
	LOG_LOGD("Reschedule for  %s", pJob->appId.c_str());
	if (pJob == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return;
	}

	account_sync_state_e syncSupport;
	int ret = account_get_sync_support(pJob->account,  &syncSupport);
	if (ret < 0)
	{
		LOG_LOGD("account access failed, returning");
		return;
	}
	if (syncSupport == (int)ACCOUNT_SYNC_INVALID || syncSupport == (int)ACCOUNT_SYNC_NOT_SUPPORT)
	{
		LOG_LOGD("this account does not support sync, returning");
		return;
	}

	SyncJob* pNewJob = new SyncJob(pJob->appId, pJob->account, pJob->capability, pJob->pExtras,pJob->reason, pJob->syncSource, DELAY_RETRY_SYNC_IN_PROGRESS_IN_SECONDS * 1000,
								   pJob->flexTime,pJob->backoff, pJob->delayUntil, pJob->isParallelSyncAllowed);
	if (pJob == NULL)
	{
		LOG_LOGD("Failed to construct SyncJob");
		return;
	}
	ScheduleSyncJob(pNewJob);
	delete pNewJob;
	pNewJob = NULL;
}

void
SyncManager::SetDelayTimeValue(SyncJob* pJob, long delayUntilSeconds)
{
	if (pJob == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return;
	}
	long long delayUntil = delayUntilSeconds * 1000LL;

	time_t  absoluteNow;
	time(&absoluteNow);

	long newDelayUntilTime;
	if (delayUntil > absoluteNow)
	{
		long long elapsedTime = GetElapsedTime();
		newDelayUntilTime = elapsedTime + (delayUntil - absoluteNow);
	}
	else
	{
		newDelayUntilTime = 0;
	}
	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncJobQueue->OnDelayUntilTimeChanged(pJob->account, pJob->capability, newDelayUntilTime);
	pthread_mutex_unlock(&__syncJobQueueMutex);
}

bool
SyncManager::IsJobActive(CurrentSyncContext *pCurrSync)
{
	if (pCurrSync == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return false;
	}
	pthread_mutex_lock(&__currJobQueueMutex);
	bool ret =  __pCurrentSyncJobQueue->IsJobActive(pCurrSync);
	pthread_mutex_unlock(&__currJobQueueMutex);
	return ret;
}

//}//_SyncManager
