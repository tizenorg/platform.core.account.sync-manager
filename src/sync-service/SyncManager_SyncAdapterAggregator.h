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
 * @file	SyncManager_SyncAdapterAggregator.h
 * @brief	This is the header file for the SyncManager class.
 */

#ifndef SYNC_SERVICE_SYNC_ADAPTER_AGGREGATOR_H
#define SYNC_SERVICE_SYNC_ADAPTER_AGGREGATOR_H

#include <iostream>
#include <list>
#include <bundle.h>
#include <stdio.h>
#include <account.h>
#include <package-manager.h>
#include "SyncManager_SyncJobQueue.h"
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_NetworkChangeListener.h"
#include "SyncManager_StorageChangeListener.h"
#include "SyncManager_BatteryStatusListener.h"
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncWorker.h"
#include "SyncManager_Singleton.h"
#include "SyncManager_CurrentSyncJobQueue.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/
class RepositoryEngine;
class SyncAdapter;

using namespace std;

class SyncAdapterAggregator
{
public:
	void AddSyncAdapter(const char* pAccoutProviderAppId, const char* pServiceAppId, const char* pCapability);

	bool HasSyncAdapter(const char* pAccountProviderId, const char* pServiceAppId, const char* pCapability);

	void RemoveSyncAdapter(const char* pServiceAppId, const char* pCapability);

	char* GetSyncAdapter(account_h account, string capability);

	char* GetSyncAdapter(const char* pAppId);

	void HandlePackageUninstalled(const char* pAppId);

	void dumpSyncAdapters();

protected:
	SyncAdapterAggregator(void);

	~SyncAdapterAggregator(void);

	friend class Singleton<SyncManager>;


class SyncAdapter
{
public:
	SyncAdapter(const char* pAccoutProviderAppId, const char* pServiceAppId, const char* pCapability)
	{
		__pAccountProviderId = strdup(pAccoutProviderAppId);
		__pCapability = (pCapability == NULL)? NULL : strdup(pCapability);
		__pAppId = strdup(pServiceAppId);
	}

	~SyncAdapter(void)
	{
		if (__pAccountProviderId)
		{
			free(__pAccountProviderId);
		}

		if (__pCapability)
		{
			free(__pCapability);
		}

		if (__pAppId)
		{
			free(__pAppId);
		}
	}
private:

	SyncAdapter(const SyncAdapter&);

	const SyncAdapter& operator=(const SyncAdapter&);

public:
	char* __pAccountProviderId;
	char* __pCapability;
	char* __pAppId;
};



private:

	SyncAdapterAggregator(const SyncAdapterAggregator&);

	const SyncAdapterAggregator& operator=(const SyncAdapterAggregator&);

private:
	multimap<string, SyncAdapter*> __syncAdapterList;

	friend class SyncManager;
	friend class RepositoryEngine;
};
//}//_SyncManager
#endif //SYNC_SERVICE_SYNC_ADAPTER_AGGREGATOR_H
