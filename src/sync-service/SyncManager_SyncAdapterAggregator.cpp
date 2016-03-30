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


//LCOV_EXCL_START
SyncAdapterAggregator::~SyncAdapterAggregator(void)
{
}
//LCOV_EXCL_STOP


void
SyncAdapterAggregator::AddSyncAdapter(const char* pPackageId, const char* pServiceAppId)
{
	if (HasSyncAdapter(pPackageId)) {
		LOG_LOGD("Sync adapter already registered for package [%s]", pPackageId);	//LCOV_EXCL_LINE
	} else {
		LOG_LOGD("Registering sync-adapter [%s] for package [%s]", pServiceAppId, pPackageId);
		__syncAdapterList.insert(std::pair<string, string> (pPackageId, pServiceAppId));
	}
}


void
SyncAdapterAggregator::dumpSyncAdapters()
{
/*
	for (multimap<string, SyncAdapter*>::iterator it = __syncAdapterList.begin(); it != __syncAdapterList.end(); ++it)
	{
		SyncAdapter* pSyncAdapter = it->second;
		LOG_LOGD("account provider ID %s => service app Id %s & syncJobName %s", (*it).first.c_str(), pSyncAdapter->__pAppId, pSyncAdapter->__pCapability);
	}
*/
}


const char*
SyncAdapterAggregator::GetSyncAdapter(const char* pAppId)
{
	string PkgId(pAppId);
	if (PkgId.empty()) {
		//PkgId = SyncManager::GetInstance()->GetPkgIdByCommandline(pAppId);
		if (PkgId.empty())	//LCOV_EXCL_LINE
			return NULL;
	}

	map<string, string>::iterator it = __syncAdapterList.find(PkgId.c_str());
	if (it != __syncAdapterList.end()) {
		return it->second.c_str();
	}

	LOG_LOGD("Sync adapter not found for account provider id %s", pAppId);

	return NULL;
}


//LCOV_EXCL_START
bool
SyncAdapterAggregator::HasServiceAppId(const char* pAccountProviderId)
{
	bool result = false;
/*
	pair<multimap<string, SyncAdapter*>::iterator, multimap<string, SyncAdapter*>::iterator> ret;
	ret = __syncAdapterList.equal_range(pAccountProviderId);

	for(multimap<string, SyncAdapter*>::iterator it = ret.first; it != ret.second; ++it)
	{
		LOG_LOGD("Sync Adapter is found by using caller package name successfully");
		result = true;
	}
*/

	return result;
}
//LCOV_EXCL_STOP


bool
SyncAdapterAggregator::HasSyncAdapter(const char* pPackageId)
{
	map<string, string>::iterator it = __syncAdapterList.find(pPackageId);
	return it != __syncAdapterList.end();
}


//LCOV_EXCL_START
void
SyncAdapterAggregator::HandlePackageUninstalled(const char* pPackageId)
{
	LOG_LOGD("Removing sync adapter for package [%s]", pPackageId);
	__syncAdapterList.erase(pPackageId);
}
//LCOV_EXCL_STOP


void
SyncAdapterAggregator::RemoveSyncAdapter(const char* pPackageId)
{
	__syncAdapterList.erase(pPackageId);
}

//}//_SyncManager
