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
#include <app_manager.h>
#include <tzplatform_config.h>
#include "sync-error.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncAdapterAggregator.h"
#include "SyncManager_SyncJobsAggregator.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_PeriodicSyncJob.h"
#include "SyncManager_DataSyncJob.h"
#include "sync_manager.h"


/*namespace _SyncManager
{*/

#define VCONF_HOME_SCREEN  "db/setting/homescreen/package_name"
#define VCONF_LOCK_SCREEN  "file/private/lockscreen/pkgname"

#define SYNC_DATA_DIR tzplatform_mkpath(TZ_USER_DATA, "/sync-manager")

int DELAY_RETRY_SYNC_IN_PROGRESS_IN_SECONDS = 10;
#define ID_FOR_ACCOUNT_LESS_SYNC -2


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
SyncManager::AddOnDemandSync(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId)
{
	const char* pSyncAdapterApp = __pSyncAdapterAggregator->GetSyncAdapter(pPackageId.c_str());
	SYNC_LOGE_RET_RES(pSyncAdapterApp != NULL, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND, "Sync adapter cannot be found for package %s", pPackageId.c_str());

	if (accountId != -1 && !GetSyncSupport(accountId))
	{
		LOG_LOGD("Sync is not enabled for account ID %s", accountId);

		return SYNC_ERROR_SYSTEM;
	}

	SyncJob* pJob = new (std::nothrow) SyncJob(pSyncAdapterApp, syncJobName, accountId, pExtras, syncOption, syncJobId, SYNC_TYPE_ON_DEMAND);
	SYNC_LOGE_RET_RES(pJob != NULL, SYNC_ERROR_OUT_OF_MEMORY, "Failed to construct SyncJob");

	__pSyncJobsAggregator->AddSyncJob(pPackageId.c_str(), syncJobName, pJob);

	ScheduleSyncJob(pJob);

	return SYNC_ERROR_NONE;
}


int
SyncManager::CancelSync(SyncJob* pSyncJob)
{
	LOG_LOGD("SyncManager::CancelSync Starts");

	ClearScheduledSyncJobs(pSyncJob);
	CancelActiveSyncJob(pSyncJob);

	LOG_LOGD("SyncManager::CancelSync Ends");

	return SYNC_ERROR_NONE;
}


int
SyncManager::AddPeriodicSyncJob(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId, long period)
{
	if (period < 1800)
	{
		LOG_LOGD("Requested period %d is less than minimum, rounding up to 30 mins", period);

		period = 1800;
	}

	const char* pSyncAdapterApp = __pSyncAdapterAggregator->GetSyncAdapter(pPackageId.c_str());
	SYNC_LOGE_RET_RES(pSyncAdapterApp != NULL, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND, "Sync adapter cannot be found for package %s", pPackageId.c_str());
	LOG_LOGD("Found sync adapter [%s]", pSyncAdapterApp);

	if (accountId != -1 && !GetSyncSupport(accountId))
	{
		LOG_LOGD("Sync is not enabled for account ID %s", accountId);
		return SYNC_ERROR_SYSTEM;
	}

	PeriodicSyncJob* pRequestedJob = new (std::nothrow) PeriodicSyncJob(pSyncAdapterApp, syncJobName, accountId, pExtras, syncOption, syncJobId, SYNC_TYPE_PERIODIC, period / 60);
	SYNC_LOGE_RET_RES(pRequestedJob != NULL, SYNC_ERROR_OUT_OF_MEMORY, "Failed to construct periodic SyncJob");

	__pSyncJobsAggregator->AddSyncJob(pPackageId.c_str(), syncJobName, pRequestedJob);
	__pPeriodicSyncScheduler->SchedulePeriodicSyncJob(pRequestedJob);
	if (pRequestedJob->IsExpedited())
	{
		ScheduleSyncJob(pRequestedJob);
	}

	return SYNC_ERROR_NONE;
}


int
SyncManager::AddDataSyncJob(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId, const char* pCapability)
{
	const char* pSyncAdapterApp = __pSyncAdapterAggregator->GetSyncAdapter(pPackageId.c_str());
	SYNC_LOGE_RET_RES(pSyncAdapterApp != NULL, SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND, "Sync adapter cannot be found for package %s", pPackageId.c_str());
	LOG_LOGD("Found sync adapter [%s]", pSyncAdapterApp);

	if (accountId != -1 && !GetSyncSupport(accountId))
	{
		LOG_LOGD("Sync is not enabled for account ID %s", accountId);
		return SYNC_ERROR_SYSTEM;
	}

	DataSyncJob* pRequestedJob = new (std::nothrow) DataSyncJob(pSyncAdapterApp, syncJobName, accountId, pExtras, syncOption, syncJobId, SYNC_TYPE_DATA_CHANGE, pCapability);
	SYNC_LOGE_RET_RES(pRequestedJob != NULL, SYNC_ERROR_OUT_OF_MEMORY, "Failed to construct periodic SyncJob");

	__pSyncJobsAggregator->AddSyncJob(pPackageId.c_str(), syncJobName, pRequestedJob);
	__pDataChangeSyncScheduler->AddDataSyncJob(syncJobName, pRequestedJob);
	if (pRequestedJob->IsExpedited())
	{
		ScheduleSyncJob(pRequestedJob);
	}

	return SYNC_ERROR_NONE;
}


int
SyncManager::RemoveSyncJob(string packageId, int syncJobId)
{
	LOG_LOGD("Starts");
	int ret = SYNC_ERROR_NONE;

	ISyncJob* pSyncJob = __pSyncJobsAggregator->GetSyncJob(packageId.c_str(), syncJobId);
	SYNC_LOGE_RET_RES(pSyncJob != NULL, SYNC_ERROR_UNKNOWN, "Sync job for id [%d] doesnt exist or already removed", syncJobId);

	SyncType syncType = pSyncJob->GetSyncType();
	if (syncType == SYNC_TYPE_DATA_CHANGE)
	{
		DataSyncJob* dataSyncJob = dynamic_cast< DataSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(dataSyncJob != NULL, SYNC_ERROR_SYSTEM, "Failed to cast %d", syncJobId);

		__pDataChangeSyncScheduler->RemoveDataSyncJob(dataSyncJob);
	}
	else if(syncType == SYNC_TYPE_PERIODIC)
	{
		PeriodicSyncJob* periodicSyncJob = dynamic_cast< PeriodicSyncJob* > (pSyncJob);
		SYNC_LOGE_RET_RES(periodicSyncJob != NULL, SYNC_ERROR_SYSTEM, "Failed to cast %d", syncJobId);

		ret = __pPeriodicSyncScheduler->RemoveAlarmForPeriodicSyncJob(periodicSyncJob);
		SYNC_LOGE_RET_RES(ret == SYNC_ERROR_NONE, SYNC_ERROR_SYSTEM, "Failed to remove %d", syncJobId);
	}

	SyncJob* pJob = dynamic_cast< SyncJob* > (pSyncJob);
	CancelSync(pJob);

	__pSyncJobsAggregator->RemoveSyncJob(packageId.c_str(), syncJobId);

	return ret;
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
	return __pSyncJobQueue->AddSyncJob(pJob);
}


void
SyncManager::AddRunningAccount(int account_id, int pid)
{
	__runningAccounts.insert(make_pair(account_id, pid));
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
#if !defined(_SEC_FEATURE_CONTAINER_ENABLE)
	__runningAccounts.clear();
	if (account_foreach_account_from_db(accountCb, this) < 0)
	{
		LOG_LOGD("UpdateRunningAccounts: Can not fetch account from db");
	}
#endif
}


#if !defined(_SEC_FEATURE_CONTAINER_ENABLE)
bool OnAccountUpdated(const char* pEventType, int acountId, void* pUserData)
{
	//TODO: will go in enhancements
	SyncManager* pSyncManager = (SyncManager*)pUserData;
	pSyncManager->UpdateRunningAccounts();

	//pSyncManager->ScheduleSync(NULL, NULL, REASON_ACCOUNT_UPDATED, NULL, 0, 0, false);

	return true;
}
#endif


void
SyncManager::OnDNetStatusChanged(bool connected)
{
	LOG_LOGD("Data network change detected %d", connected);

	//bool wasConnected = __isSimDataConnectionPresent;
	__isSimDataConnectionPresent = connected;
	if (__isSimDataConnectionPresent)
	{
		SendSyncCheckAlarmMessage();
	}
}


void
SyncManager::OnWifiStatusChanged(bool connected)
{
	LOG_LOGD("Wifi network change detected %d", connected);

	//bool wasConnected = __isWifiConnectionPresent;
	__isWifiConnectionPresent = connected;
	if (__isWifiConnectionPresent)
	{
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


static int OnPackageUninstalled(unsigned int userId, int reqId, const char* pPkgType, const char* pPkgId, const char* pKey,	const char* pVal, const void* pMsg, void* pData)
{
	LOG_LOGD("OnPackageUninstalled [type %s] type [pkdId:%s]", pPkgType, pPkgId);
	if (!strcmp("end", pKey) && !strcmp("ok", pVal))
	{
		SyncManager::GetInstance()->GetSyncAdapterAggregator()->HandlePackageUninstalled(pPkgId);
		SyncManager::GetInstance()->GetSyncJobsAggregator()->HandlePackageUninstalled(pPkgId);
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
	if (pCommandLine != NULL)
	{
		char cmd[100];
		memset(cmd, 0x00, sizeof(cmd));
		snprintf(cmd, sizeof(cmd), "rpm -qf %s --queryformat '%%{name}\\t'", pCommandLine);

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
	}

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
	if (__pDataChangeSyncScheduler)
	{
		if(!__pDataChangeSyncScheduler->RegisterDataChangeListeners())
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
	if (__pDataChangeSyncScheduler)
	{
		return (__pDataChangeSyncScheduler->DeRegisterDataChangeListeners());
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
SyncManager::ClearScheduledSyncJobs(SyncJob* pSyncJob)
{
	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncJobQueue->RemoveSyncJob(pSyncJob);
	pthread_mutex_unlock(&__syncJobQueueMutex);
}


void
SyncManager::CancelActiveSyncJob(SyncJob* pSyncJob)
{
	pthread_mutex_lock(&__currJobQueueMutex);
	CurrentSyncContext *pCurrSyncContext = __pCurrentSyncJobQueue->GetCurrJobfromKey(pSyncJob->__key);
	pthread_mutex_unlock(&__currJobQueueMutex);
	if (pCurrSyncContext != NULL)
	{
		g_source_remove(pCurrSyncContext->GetTimerId());
		CloseCurrentSyncContext(pCurrSyncContext);
		SendCancelSyncsMessage(pSyncJob);
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
	, __pDataChangeSyncScheduler(NULL)
	, __pPeriodicSyncScheduler(NULL)
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
	LOG_LOGE_BOOL(ret == VCONF_OK, "vconf_get_int failed %d", ret);
	__isStorageLow = (storageState == LOW_MEMORY_NORMAL) ? false : true;

	int upsMode;

	if (-1 == access (SYNC_DATA_DIR, F_OK)) {
		mkdir(SYNC_DATA_DIR, 755);
	}

	ret = vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &upsMode);
	LOG_LOGE_BOOL(ret == VCONF_OK, "vconf_get_int failed %d", ret);
	__isUPSModeEnabled = (upsMode == SETTING_PSMODE_EMERGENCY) ? true : false;

	__pNetworkChangeListener = new (std::nothrow) NetworkChangeListener();
	LOG_LOGE_BOOL(__pNetworkChangeListener, "Failed to construct NetworkChangeListener");

	__pStorageListener = new (std::nothrow) StorageChangeListener();
	LOG_LOGE_BOOL(__pStorageListener, "Failed to construct StorageChangeListener");

	__pBatteryStatusListener = new (std::nothrow) BatteryStatusListener();
	LOG_LOGE_BOOL(__pBatteryStatusListener, "Failed to construct BatteryStatusListener");

	__pDataChangeSyncScheduler = new (std::nothrow) DataChangeSyncScheduler();
	LOG_LOGE_BOOL(__pDataChangeSyncScheduler, "Failed to Construct DataChangeSyncScheduler");

	__pPeriodicSyncScheduler = new (std::nothrow) PeriodicSyncScheduler();
	LOG_LOGE_BOOL(__pPeriodicSyncScheduler, "Failed to Construct PeriodicSyncScheduler");

	__pSyncAdapterAggregator = new (std::nothrow) SyncAdapterAggregator();
	LOG_LOGE_BOOL(__pSyncAdapterAggregator, "Failed to construct SyncAdapterAggregator");

	__pSyncJobsAggregator = new (std::nothrow) SyncJobsAggregator();
	LOG_LOGE_BOOL(__pSyncJobsAggregator, "Failed to construct SyncJobsAggregator");

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

	LOG_LOGD("Register event listeners");
	RegisterForNetworkChange();
	RegisterForStorageChange();
	RegisterForBatteryStatus();
	RegisterForUPSModeChange();
	RegisterForDataChange();

	LOG_LOGE_BOOL(pthread_mutex_init(&__syncJobQueueMutex, NULL) == 0, "__syncJobQueueMutex init failed");
	LOG_LOGE_BOOL(pthread_mutex_init(&__currJobQueueMutex, NULL) == 0, "__currJobQueueMutex init failed");

	__pPkgmgrClient = pkgmgr_client_new(PC_LISTENING);
	LOG_LOGE_BOOL(__pPkgmgrClient != NULL, "__pPkgmgrClient is null");

	LOG_LOGE_BOOL(SetPkgMgrClientStatusChangedListener() == 0, "Failed to register for uninstall callback.");

/*
#if !defined(_SEC_FEATURE_CONTAINER_ENABLE)
	UpdateRunningAccounts();

	if (account_subscribe_create(&__accountSubscriptionHandle) < 0)
	{
		LOG_LOGD("Failed to create account subscription handle");
	}
	else if (account_subscribe_notification(__accountSubscriptionHandle, OnAccountUpdated, this) < 0)
	{
		LOG_LOGD("Failed to register callback for account updation");
	}
#endif
*/

	Initialize();

	__pSyncRepositoryEngine->OnBooting();
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
	delete __pDataChangeSyncScheduler;
	delete __pPeriodicSyncScheduler;
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


SyncAdapterAggregator*
SyncManager::GetSyncAdapterAggregator()
{
	return __pSyncAdapterAggregator;
}


SyncJobsAggregator*
SyncManager::GetSyncJobsAggregator()
{
	return __pSyncJobsAggregator;
}

void
SyncManager::HandleShutdown(void)
{
	pthread_mutex_lock(&__syncJobQueueMutex);
	__pSyncRepositoryEngine->SaveCurrentState();
	pthread_mutex_unlock(&__syncJobQueueMutex);
}


bool
SyncManager::GetSyncSupport(int accountId)
{
	account_h accountHandle = NULL;
	int ret = account_create(&accountHandle);
	LOG_LOGE_BOOL(ret == ACCOUNT_ERROR_NONE, "account access failed [%d]", ret);

	KNOX_CONTAINER_ZONE_ENTER(GetAccountPid(accountId));
	ret =  account_query_account_by_account_id(accountId, &accountHandle);
	KNOX_CONTAINER_ZONE_EXIT();
	LOG_LOGE_BOOL(ret == ACCOUNT_ERROR_NONE, "account query failed [%d]", ret);

	account_sync_state_e syncSupport;
	ret = account_get_sync_support(accountHandle,  &syncSupport);
	LOG_LOGE_BOOL(ret == ACCOUNT_ERROR_NONE, "account access failed [%d]", ret);

	if (syncSupport == ACCOUNT_SYNC_INVALID || syncSupport == ACCOUNT_SYNC_NOT_SUPPORT)
	{
		LOG_LOGD("The account does not support sync");
		return false;
	}

	return true;
}


void
SyncManager::SendCancelSyncsMessage(SyncJob* pJob)
{
	LOG_LOGD("SyncManager::SendCancelSyncsMessage :sending MESSAGE_CANCEL");
	Message msg;
	msg.type = SYNC_CANCEL;
	msg.pSyncJob = new SyncJob(*pJob);
	FireEvent(__pSyncJobDispatcher, msg);
}


void
SyncManager::OnResultReceived(SyncStatus res, string appId, string packageId, const char* syncJobName)
{
	string key;
	key.append("id:").append(appId).append(syncJobName);

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
		SyncJob* pJob = pCurrSyncContext->GetSyncJob();
		SendSyncCompletedOrCancelledMessage(pJob, res);
		CloseCurrentSyncContext(pCurrSyncContext);

		if (res == SYNC_STATUS_SUCCESS && pJob->GetSyncType() == SYNC_TYPE_ON_DEMAND)
		{
			LOG_LOGD("On demand sync completed. Deleting the job %s", key.c_str());
			__pSyncJobsAggregator->RemoveSyncJob(packageId.c_str(), syncJobName);
		}
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
	__pCurrentSyncJobQueue->RemoveSyncContextFromCurrentSyncQueue(activeSyncContext);
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
SyncManager::ScheduleSyncJob(SyncJob* pJob, bool fireCheckAlarm)
{
	int err;
	pthread_mutex_lock(&__syncJobQueueMutex);
	err = __pSyncJobQueue->AddSyncJob(pJob);
	pthread_mutex_unlock(&__syncJobQueueMutex);

	if (err == SYNC_ERROR_NONE)
	{
		if(fireCheckAlarm)
		{
			LOG_LOGD("Added sync job [%s] to Main queue, Intiating dispatch sequence", pJob->__key.c_str());
			SendSyncCheckAlarmMessage();
		}
	}
	else if (err == SYNC_ERROR_ALREADY_IN_PROGRESS)
	{
		LOG_LOGD("Duplicate sync job [%s], No need to enqueue", pJob->__key.c_str());
	}
	else
	{
		LOG_LOGD("Failed to add into sync job list");
	}
}


void
SyncManager::TryToRescheduleJob(SyncStatus syncResult, SyncJob* pJob)
{
	if (pJob == NULL)
	{
		LOG_LOGD("Invalid parameter");
		return;
	}
	LOG_LOGD("Reschedule for  %s", pJob->__appId.c_str());

	if (syncResult == SYNC_STATUS_FAILURE || syncResult == SYNC_STATUS_CANCELLED)
	{
		if (!pJob->IsNoRetry())
		{
			ScheduleSyncJob(pJob, false);
		}
	}
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
