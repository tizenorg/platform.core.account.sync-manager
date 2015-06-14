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
 * @brief	This is the header file for the SyncService class.
 */


#ifndef SYNC_SERVICE_SYNC_SERVICE_H
#define SYNC_SERVICE_SYNC_SERVICE_H

#include <iostream>
#include <bundle.h>
#include "SyncManager_Singleton.h"
#include "sync-adapter-stub.h"
#include "account.h"


/*namespace _SyncManager
{
*/

class SyncManager;

class SyncService
		: public Singleton<SyncService>
{

public:

	int StartService(void);

	void TriggerStartSync(const char* appId,int accountId, bundle* pExtras, const char* pCapability);

	void TriggerStopSync(const char* appId, account_h account, const char* pCapability);

	void RequestSync(const char* appId, int accountId, bundle* pExtras, const char* pCapability);

	void CancelSync(const char* appId, account_h account, const char* pCapability);

	void AddPeriodicSyncJob(const char* appId, int accountId, bundle* pExtras, const char* pCapability, unsigned long pollFrequency);

	void RemovePeriodicSyncJob(const char* appId, account_h account, bundle* pExtras, const char* pCapability);

	void HandleShutdown(void);

protected:
	SyncService(void);

	~SyncService(void);

	friend class Singleton<SyncService>;

private:
	SyncService(const SyncService&);

	const SyncService& operator=(const SyncService&);

	void InitializeDbus();
};
//}//_SyncManager
#endif //SYNC_SERVICE_SYNC_SERVICE_H
