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
{
	for (unsigned int i=0; i<periodicSyncList.size(); i++)
	{
		delete periodicSyncList[i];
	}
}


// Create an authority with one periodic sync scheduled with empty bundle and syncing every day.
CapabilityInfo::CapabilityInfo(string appId, account_h account, string capability, int id)
{
	this->appId = appId;
	this->accountHandle = account;
	this->capability = capability;
	this->id = id;
	isEnabled = true;
	syncable = -1;
	backOffTime = -1;
	backOffDelay = -1;
}


CapabilityInfo::CapabilityInfo(const CapabilityInfo& capabilityInfo)
{
	this->appId =  capabilityInfo.appId;
	this->accountHandle = capabilityInfo.accountHandle;
	this->capability = capabilityInfo.capability;
	this->id = capabilityInfo.id;
	this->isEnabled = capabilityInfo.isEnabled;
	this->syncable = capabilityInfo.syncable;
	this->backOffTime = capabilityInfo.backOffTime;
	this->backOffDelay = capabilityInfo.backOffDelay;
	this->delayUntil = capabilityInfo.delayUntil;

	for (unsigned int i = 0; i < capabilityInfo.periodicSyncList.size(); i++)
	{
		PeriodicSyncJob* pJob  = new PeriodicSyncJob(*(capabilityInfo.periodicSyncList[i]));
		if (pJob)
		{
			(this->periodicSyncList).push_back(pJob);
		}
	}
}

CapabilityInfo& CapabilityInfo::operator =(const CapabilityInfo& capabilityInfo)
{
	this->appId =  capabilityInfo.appId;
	this->accountHandle = capabilityInfo.accountHandle;
	this->capability = capabilityInfo.capability;
	this->id = capabilityInfo.id;
	this->isEnabled = capabilityInfo.isEnabled;
	this->syncable = capabilityInfo.syncable;
	this->backOffTime = capabilityInfo.backOffTime;
	this->backOffDelay = capabilityInfo.backOffDelay;
	this->delayUntil = capabilityInfo.delayUntil;

	for (unsigned int i = 0; i < capabilityInfo.periodicSyncList.size(); i++)
	{
		PeriodicSyncJob* pJob = new PeriodicSyncJob(*(capabilityInfo.periodicSyncList[i]));
		if (pJob)
		{
			this->periodicSyncList.push_back(pJob);
		}
	}
	return *this;
}

//}//_SyncManager
