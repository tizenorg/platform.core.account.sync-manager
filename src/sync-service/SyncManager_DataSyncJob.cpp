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
 * @file	SyncManager_DataSyncJob.cpp
 * @brief	This is the implementation file for the DataSyncJob class.
 */

#include "SyncManager_SyncManager.h"
#include "SyncManager_DataSyncJob.h"

/*namespace _SyncManager
{*/


DataSyncJob::~DataSyncJob(void)
{
}


DataSyncJob::DataSyncJob(const string appId, const string syncJobName, int accountId, bundle* pUserData, int syncOption, int syncJobId, SyncType type, string capability)
	: SyncJob(appId, syncJobName, accountId, pUserData, syncOption, syncJobId, type)
	, __capability(capability)
{
}


//LCOV_EXCL_START
DataSyncJob::DataSyncJob(const DataSyncJob& other)
	: SyncJob(other)
{
/*
	this->__accountHandle = other.__accountHandle;
	this->__capability = other.__capability;
	this->__pExtras = bundle_dup(other.__pExtras);
	this->__period = other.__period;
	this->__syncAdapter = other.__syncAdapter;
*/
}


DataSyncJob&
DataSyncJob::operator= (const DataSyncJob& other)
{
/*
	this->__accountHandle = other.__accountHandle;
	this->__capability = other.__capability;
	this->__pExtras = bundle_dup(other.__pExtras);
	this->__period = other.__period;
	this->__syncAdapter = other.__syncAdapter;
*/

	return *this;
}


void
DataSyncJob::Reset(int accountId, bundle* pUserData, int syncOption, string capability)
{
	SyncJob::Reset(accountId, pUserData, syncOption);
	__capability = capability;
}
//LCOV_EXCL_STOP

//}//_SyncManager
