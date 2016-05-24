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
 * @file	SyncManager_SyncManager.h
 * @brief	This is the header file for the SyncManager class.
 */

#ifndef SYNC_SERVICE_SYNC_MANAGER_H
#define SYNC_SERVICE_SYNC_MANAGER_H

#include <iostream>
#include <list>
#include <bundle.h>
#include <bundle_internal.h>
#include <stdio.h>
#include <account.h>
#include <package-manager.h>
#include <media_content_type.h>
#include "sync_manager.h"
#include "SyncManager_SyncJobQueue.h"
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_NetworkChangeListener.h"
#include "SyncManager_StorageChangeListener.h"
#include "SyncManager_BatteryStatusListener.h"
#include "SyncManager_DataChangeSyncScheduler.h"
#include "SyncManager_PeriodicSyncScheduler.h"
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncWorker.h"
#include "SyncManager_Singleton.h"
#include "SyncManager_CurrentSyncJobQueue.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_ManageIdleState.h"


/*namespace _SyncManager
{
*/

struct backOff;
class SyncStatusInfo;
class RepositoryEngine;
class SyncJob;
class SyncAdapterAggregator;
class SyncJobsAggregator;

using namespace std;

class SyncManager
		: public SyncWorker, public Singleton < SyncManager > {
public:
	void SetSyncSetting(bool enable);

	bool GetSyncSetting();

	int AddOnDemandSync(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId);

	int CancelSync(SyncJob* pJob);

	int AddPeriodicSyncJob(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId, long period);

	int AddDataSyncJob(string pPackageId, const char* syncJobName, int accountId, bundle* pExtras, int syncOption, int syncJobId, const char* pCapability);

	int RemoveSyncJob(string packageId, int syncJobId);

	SyncJobQueue* GetSyncJobQueue(void) const;

	int AddToSyncQueue(SyncJob* pJob);

	/* Callback on wifi, ethernet, cellular, bt and wifidirect status change */
	void OnDNetStatusChanged(bool connected);

	void OnWifiStatusChanged(bool connected);

	void OnEthernetStatusChanged(bool connected);

	void OnBluetoothStatusChanged(bool connected);

	void OnStorageStatusChanged(int value);

	void OnBatteryStatusChanged(int value);

	void OnUPSModeChanged(bool enable);

	RepositoryEngine* GetSyncRepositoryEngine(void);

	bool AreAccountsEqual(account_h account1, account_h account2);

	bool GetSyncSupport(int accountId);

	SyncAdapterAggregator* GetSyncAdapterAggregator();

	SyncJobsAggregator* GetSyncJobsAggregator();

	ManageIdleState* GetManageIdleState();

	void AddSyncAdapter(string packageId, string svcAppId);

	void AddRunningAccount(int account_id, int pid);

	int GetAccountPid(int account_id);

	void UpdateRunningAccounts(void);

	void ScheduleSyncJob(SyncJob* pJob, bool fireCheckAlarm = true);

	void SendSyncCompletedOrCancelledMessage(SyncJob *pJob, int result);

	void AlertForChange();

	void OnResultReceived(SyncStatus res, string svcAppId, string packageId, const char* syncJobName);

	string GetPkgIdByAppId(const char* pAppId);

	string GetPkgIdByCommandline(const char* pCommandLine);

	void HandleShutdown(void);

	void RecordSyncAdapter(void);

	void RecordSyncJob(void);

	void CloseCurrentSyncContext(CurrentSyncContext *activeSyncContext);

protected:
	SyncManager(void);

	~SyncManager(void);

	friend class Singleton < SyncManager > ;

private:
	bool Construct(void);

	void Destruct(void);

	void RegisterForUPSModeChange(void);

	int DeRegisterForUPSModeChange(void);

	void RegisterForNetworkChange(void);

	int DeRegisterForNetworkChange(void);

	void RegisterForStorageChange(void);

	int DeRegisterForStorageChange(void);

	void RegisterForBatteryStatus(void);

	int DeRegisterForBatteryStatus(void);

	void RegisterForDataChange(void);

	int DeRegisterForDataChange(void);

	int SetPkgMgrClientStatusChangedListener(void);

	void ClearScheduledSyncJobs(SyncJob* pSyncJob);

	void CancelActiveSyncJob(SyncJob* pSyncJob);

	bool IsActiveAccount(vector < account_h > accounts, account_h account);

	void TryToRescheduleJob(SyncStatus syncResult, SyncJob* pJob);

	bool IsJobActive(CurrentSyncContext *pCurrSync);

	void SendCancelSyncsMessage(SyncJob* pSyncJob);

	void SendSyncAlarmMessage();

	void SendSyncCheckAlarmMessage();

	bool GetBundleVal(const char* pKey);

private:
	SyncManager(const SyncManager&);

	const SyncManager &operator = (const SyncManager&);

private:
	bool __isStorageLow;
	bool __isWifiConnectionPresent;
	bool __isEthernetConnectionPresent;
	bool __isSimDataConnectionPresent;
	bool __isUPSModeEnabled;
	bool __isSyncPermitted;

	ManageIdleState* __pManageIdleState;
	NetworkChangeListener* __pNetworkChangeListener;
	StorageChangeListener* __pStorageListener;
	BatteryStatusListener* __pBatteryStatusListener;
	DataChangeSyncScheduler* __pDataChangeSyncScheduler;
	PeriodicSyncScheduler* __pPeriodicSyncScheduler;

	RepositoryEngine* __pSyncRepositoryEngine;
	SyncJobQueue* __pSyncJobQueue;
	SyncJobDispatcher* __pSyncJobDispatcher;
	map < string, string > __syncAdapterList;

	SyncAdapterAggregator* __pSyncAdapterAggregator;
	SyncJobsAggregator* __pSyncJobsAggregator;
	CurrentSyncJobQueue* __pCurrentSyncJobQueue;
	account_subscribe_h __accountSubscriptionHandle;
	map < int, int > __runningAccounts;
	/* vector < account_h > __runningAccounts; */

	pthread_mutex_t __syncJobQueueMutex;
	pthread_mutex_t __currJobQueueMutex;

	pkgmgr_client* __pPkgmgrClient;

	friend class SyncJobDispatcher;
	friend class RepositoryEngine;
	friend class SyncService;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_MANAGER_H */
