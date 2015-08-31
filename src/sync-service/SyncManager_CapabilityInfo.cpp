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
 * @file	SyncManager_CapabilityInfo.cpp
 * @brief	This is the implementation file for the CapabilityInfo class.
 */

#include <bundle.h>
#include "sync-log.h"
#include "SyncManager_RepositoryEngine.h"
#include "SyncManager_CapabilityInfo.h"
#include "SyncManager_SyncManager.h"


/*namespace _SyncManager
{*/


CapabilityInfo::CapabilityInfo(void)
{
	//Empty
}


CapabilityInfo::~CapabilityInfo(void)
{/*
	for (unsigned int i=0; i<periodicSyncList.size(); i++)
	{
		delete periodicSyncList[i];
	}
*/
}


CapabilityInfo::CapabilityInfo(string capability)
			: __capability(capability)
{

}


void
CapabilityInfo::AddPeriodicSyncJob(int account_id, PeriodicSyncJob* pJob)
{
	__periodicSyncList.insert(pair<int, PeriodicSyncJob*> (account_id, pJob));
}

void
CapabilityInfo::RemovePeriodicSyncJob(PeriodicSyncJob* pJob)
{
	int acc_id;
	//int ret = account_get_account_id(pJob->accountHandle, &acc_id);
	//LOG_LOGE_VOID(ret == ACCOUNT_ERROR_NONE, "app account_get_account_id failed %d", ret);

	//__periodicSyncList.erase(__periodicSyncList.find(acc_id));
}



bool
CapabilityInfo::RequestAlreadyExists(int account_id, PeriodicSyncJob* pJob)
{
	map<int, PeriodicSyncJob*>::iterator it = __periodicSyncList.find(account_id);
	if (it == __periodicSyncList.end())
	{
		return false;
	}
	PeriodicSyncJob* pSyncJob = it->second;
	if ( *pSyncJob == *pJob)
	{
		return true;
	}
	else
		return false;
}


CapabilityInfo::CapabilityInfo(const CapabilityInfo& capabilityInfo)
{
	this->__capability = capabilityInfo.__capability;

	map<int, PeriodicSyncJob*>::const_iterator endItr = capabilityInfo.__periodicSyncList.end();

	for(map<int, PeriodicSyncJob*>::const_iterator itr = capabilityInfo.__periodicSyncList.begin(); itr != endItr; ++itr)
	{
		PeriodicSyncJob* pJob  = new PeriodicSyncJob(*(itr->second));
		if (pJob)
		{
			__periodicSyncList.insert(pair<int, PeriodicSyncJob*> (itr->first, pJob));
		}
	}
}


CapabilityInfo& CapabilityInfo::operator =(const CapabilityInfo& capabilityInfo)
{
	this->__capability = capabilityInfo.__capability;

	map<int, PeriodicSyncJob*>::const_iterator endItr = capabilityInfo.__periodicSyncList.end();
	for(map<int, PeriodicSyncJob*>::const_iterator itr = capabilityInfo.__periodicSyncList.begin(); itr != endItr; ++itr)
	{
		PeriodicSyncJob* pJob  = new PeriodicSyncJob(*(itr->second));
		if (pJob)
		{
			__periodicSyncList.insert(pair<int, PeriodicSyncJob*> (itr->first, pJob));
		}
	}

	return *this;
}

//}//_SyncManager
