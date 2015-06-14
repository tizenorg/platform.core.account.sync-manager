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
 * @file	SyncManager_SyncAdapterAggregator.cpp
 * @brief	This is the implementation file for the SyncManager class.
 */

#include <sys/time.h>
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
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncAdapterAggregator.h"



/*namespace _SyncManager
{*/
using namespace std;


SyncAdapterAggregator::SyncAdapterAggregator(void)
{

}


SyncAdapterAggregator::~SyncAdapterAggregator(void)
{
	for (multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.begin(); it != __syncAdapterList.end(); ++it)
	{
		delete it->second;
	}
}


void
SyncAdapterAggregator::AddSyncAdapter(const char* pAccountProviderId, const char* pServiceAppId, const char* pCapability)
{
	if (pAccountProviderId != NULL && pServiceAppId != NULL)
	{
		if (HasSyncAdapter(pAccountProviderId, pServiceAppId, pCapability))
		{
			LOG_LOGD("Sync adapter already registered for account provider id %s and capability %s", pAccountProviderId, pCapability);
		}
		else
		{
			LOG_LOGD("Registering sync-adapter %s for [%s,%s]", pServiceAppId, pAccountProviderId, pCapability);
			SyncAdapter* pSyncAdapter = new (std::nothrow) SyncAdapter(pAccountProviderId, pServiceAppId, pCapability);
			__syncAdapterList.insert(std::pair<string, SyncAdapter*> (pAccountProviderId, pSyncAdapter));
		}
	}
	else
	{
		LOG_LOGD("Invalid parameter");
	}
}


void
SyncAdapterAggregator::dumpSyncAdapters()
{
	for (multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.begin(); it != __syncAdapterList.end(); ++it)
	{
		SyncAdapter* pSyncAdapter = it->second;
		LOG_LOGD("account provider ID %s => service app Id %s & capability %s", (*it).first.c_str(), pSyncAdapter->__pAppId, pSyncAdapter->__pCapability);
	}
}


char*
SyncAdapterAggregator::GetSyncAdapter(account_h account, string capability)
{
	char* pAccountProviderId = NULL;

	account_get_package_name(account, &pAccountProviderId);
	LOG_LOGE_NULL(pAccountProviderId != NULL, "Account provider app id not found");

	string pkgId = SyncManager::GetInstance()->GetPkgIdByAppId(pAccountProviderId);

	pair<multimap<string, SyncAdapter*>::iterator, multimap<string, SyncAdapter*>::iterator> ret;

	ret = __syncAdapterList.equal_range(pkgId.c_str());

	for(multimap<string, SyncAdapter*>::iterator it = ret.first; it != ret.second; ++it)
	{
		SyncAdapter* pSyncAdapter = it->second;
		if (pSyncAdapter->__pCapability != NULL && !capability.compare(pSyncAdapter->__pCapability))
		{
			free(pAccountProviderId);
			return pSyncAdapter->__pAppId;
		}
	}
	LOG_LOGD("Sync adapter not found for account provider id %s and capability %s", pAccountProviderId, capability.c_str());
	free(pAccountProviderId);
	return NULL;
}


char*
SyncAdapterAggregator::GetSyncAdapter(const char* pAppId)
{
	string PkgId = SyncManager::GetInstance()->GetPkgIdByAppId(pAppId);
	if (PkgId.empty())
	{
		PkgId = SyncManager::GetInstance()->GetPkgIdByCommandline(pAppId);
		if (PkgId.empty())
			return NULL;
	}

	multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.find(PkgId.c_str());
	if (it != __syncAdapterList.end())
	{
		SyncAdapter* pSyncAdapter = it->second;
		if (pSyncAdapter)
		{
			return pSyncAdapter->__pAppId;
		}
	}

	LOG_LOGD("Sync adapter not found for account provider id %s", pAppId);
	return NULL;
}


bool
SyncAdapterAggregator::HasSyncAdapter(const char* pAccountProviderId, const char* pServiceAppId, const char* pCapability)
{
	bool result = false;
	pair<multimap<string, SyncAdapter*>::iterator, multimap<string, SyncAdapter*>::iterator> ret;
	ret = __syncAdapterList.equal_range(pAccountProviderId);

	for(multimap<string, SyncAdapter*>::iterator it = ret.first; it != ret.second; ++it)
	{
		SyncAdapter* pSyncAdapter = it->second;
		if (!strcmp(pServiceAppId, pSyncAdapter->__pAppId))
		{
			if (pCapability == NULL)
			{
				result = true;
			}
			else if (!strcmp(pCapability, pSyncAdapter->__pCapability))
			{
				result = true;
			}
		}
	}
	return result;
}


void
SyncAdapterAggregator::HandlePackageUninstalled(const char* pAppId)
{
	multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.find(pAppId);

	if (it != __syncAdapterList.end())
	{
		int count = __syncAdapterList.count(pAppId);
		LOG_LOGD("Removing all the sync adapters for account provider: %s count: %d", pAppId, count);
		pair<multimap<string, SyncAdapter*>::iterator, multimap<string, SyncAdapter*>::iterator> ret = __syncAdapterList.equal_range(pAppId);
		for(multimap<string, SyncAdapter*>::iterator it = ret.first; it != ret.second; ++it)
		{
			delete it->second;
		}
		__syncAdapterList.erase(pAppId);
	}
	else
	{
		RemoveSyncAdapter(pAppId, NULL);
	}
}



void
SyncAdapterAggregator::RemoveSyncAdapter(const char* pServiceAppId, const char* pCapability)
{
	multimap<string, SyncAdapter*>::iterator endItr = __syncAdapterList.end();

	LOG_LOGD("Removing sync service app : %s", pServiceAppId);

	for(multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.begin(); it != endItr; ++it)
	{
		SyncAdapter* pSyncAdapter = it->second;
		if (pCapability == NULL && !strcmp(pServiceAppId, pSyncAdapter->__pAppId))
		{
			delete pSyncAdapter;
			__syncAdapterList.erase(it);
			continue;
		}
		if (pCapability != NULL && !strcmp(pServiceAppId, pSyncAdapter->__pAppId))
		{
			delete pSyncAdapter;
			__syncAdapterList.erase(it);
		}
	}
}

//}//_SyncManager
