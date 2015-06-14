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
#include "SyncManager_SyncJobQueue.h"
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_NetworkChangeListener.h"
#include "SyncManager_StorageChangeListener.h"
#include "SyncManager_BatteryStatusListener.h"
#include "SyncManager_DataChangeListener.h"
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncWorker.h"
#include "SyncManager_Singleton.h"
#include "SyncManager_CurrentSyncJobQueue.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/

struct backOff;
class SyncStatusInfo;
class RepositoryEngine;
class SyncJob;
class SyncAdapterAggregator;

using namespace std;

class SyncManager
		: public SyncWorker, public Singleton<SyncManager>
{

public:
	void SetSyncSetting(bool enable);

	bool GetSyncSetting();

	int RequestSync(string appId, int account, const char* capability, bundle* pExtras);

	int CancelSync(string appId, account_h account, const char* capability);

	int AddPeriodicSyncJob(string appId, int account, const char* capability, bundle* pExtras, long pollFrequency);

	int RemovePeriodicSync(string appId, account_h account, const char* capability, bundle* pExtras);

	SyncJobQueue* GetSyncJobQueue(void) const;

	int AddToSyncQueue(SyncJob* pJob);

	//Callback on wifi, cellular, bt and wifidirect status change
	void OnDNetStatusChanged(bool connected);

	void OnWifiStatusChanged(bool connected);

	void OnBluetoothStatusChanged(bool connected);

	void OnStorageStatusChanged(int value);

	void OnBatteryStatusChanged(int value);

	void OnCalendarDataChanged(int value);

	void OnContactsDataChanged(int value);

	void OnUPSModeChanged(bool enable);

	RepositoryEngine* GetSyncRepositoryEngine(void);

	bool AreAccountsEqual(account_h account1, account_h account2);

	int GetSyncable(account_h account, string capability);

	SyncAdapterAggregator* GetSyncAdapterAggregator();

	void AddRunningAccount(int account_id, int pid);

	int GetAccountPid(int account_id);

	void UpdateRunningAccounts(void);

	void ScheduleSync(string appId, int accountId, string capability, int reason, bundle* pExtra,
			long long flexTimeInMillis, long long runTimeInMillis, bool onlyForUnknownSyncableState);

	void ScheduleAccountLessSync(string appId, int reason, bundle* pExtras, long long flexTimeInMillis,
							long long runTimeInMillis, bool onlyForUnknownSyncableState);

	void ScheduleSyncJob(SyncJob* pJob, bool fireCheckAlarm = true);

	void SendSyncCompletedOrCancelledMessage(SyncJob *pJob, int result);

	void AlertForChange();

	void OnResultReceived(SyncStatus res, string appId, account_h account, const char* capability);

	long long GetElapsedTime(void);

	string GetPkgIdByAppId(const char* pAppId);

	string GetPkgIdByCommandline(const char* pCommandLine);

	void HandleShutdown(void);

	void CloseCurrentSyncContext(CurrentSyncContext *activeSyncContext);

protected:
	SyncManager(void);

	~SyncManager(void);

	friend class Singleton< SyncManager >;

private:
	bool Construct();

	void RegisterForUPSModeChange();

	int DeRegisterForUPSModeChange();

	void RegisterForNetworkChange(void);

	int DeRegisterForNetworkChange(void);

	void RegisterForStorageChange(void);

	int DeRegisterForStorageChange(void);

	void RegisterForBatteryStatus(void);

	int DeRegisterForBatteryStatus(void);

	void RegisterForDataChange(void);

	int DeRegisterForDataChange(void);

	int SetPkgMgrClientStatusChangedListener(void);

	int SetAppLaunchListener(void);

	void ClearScheduledSyncJobs(string appId, account_h account, string capability);

	void CancelActiveSyncJob(string appId, account_h account, string capability);

	bool IsActiveAccount(vector<account_h> accounts, account_h account);

	void ClearBackoffValue(SyncJob* pJob);

	void TryToRescheduleJob(SyncStatus syncResult, SyncJob* pJob);

	void SetDelayTimeValue(SyncJob* pJob, long delayUntilSeconds);

	bool IsJobActive(CurrentSyncContext *pCurrSync);

	void IncreaseBackoffValue(SyncJob* pJob);

	void SendCancelSyncsMessage(SyncJob* pSyncJob);

	void SendSyncAlarmMessage();

	void SendSyncCheckAlarmMessage();

	bool GetBundleVal(const char* pKey);

private:

	SyncManager(const SyncManager&);

	const SyncManager& operator=(const SyncManager&);

private:
	bool __isStorageLow;
	bool __isWifiConnectionPresent;
	bool __isSimDataConnectionPresent;
	bool __isUPSModeEnabled;
	bool __isSyncPermitted;

	NetworkChangeListener* __pNetworkChangeListener;
	StorageChangeListener* __pStorageListener;
	BatteryStatusListener* __pBatteryStatusListener;
	DataChangeListener* __pDataChangeListener;

	RepositoryEngine* __pSyncRepositoryEngine;
	SyncJobQueue* __pSyncJobQueue;
	SyncJobDispatcher* __pSyncJobDispatcher;
	SyncAdapterAggregator* __pSyncAdapterAggregator;
	CurrentSyncJobQueue* __pCurrentSyncJobQueue;
	account_subscribe_h __accountSubscriptionHandle;
	map<int, int> __runningAccounts;
	//vector<account_h> __runningAccounts;
	long __randomOffsetInMillis;

	pthread_mutex_t __syncJobQueueMutex;
	pthread_mutex_t __currJobQueueMutex;

	pkgmgr_client* __pPkgmgrClient;

	friend class SyncJobDispatcher;
	friend class RepositoryEngine;
	friend class SyncService;
};
//}//_SyncManager
#endif //SYNC_SERVICE_SYNC_MANAGER_H
