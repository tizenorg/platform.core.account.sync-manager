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
 * @file	SyncManager_CapabilityInfo.h
 * @brief	This is the header file for the CapabilityInfo class.
 */

#ifndef SYNC_SERVICE_CAPABILITY_INFO_H
#define SYNC_SERVICE_CAPABILITY_INFO_H

#include <string>
#include <vector>
#include <account.h>
#include "SyncManager_PeriodicSyncJob.h"

/*namespace _SyncManager
{
*/
using namespace std;

class CapabilityInfo
{
public:

	CapabilityInfo(void);

	~CapabilityInfo(void);

	CapabilityInfo(string appId, account_h account, string capability, int id);

	CapabilityInfo(const CapabilityInfo& accountInfo);

	CapabilityInfo& operator =(const CapabilityInfo& capabilityInfo);

public:
	string appId;
	account_h accountHandle;
	string capability;
	bool isEnabled;
	int syncable;
	int id;
	long backOffTime;
	long backOffDelay;
	long delayUntil;
	vector<PeriodicSyncJob*> periodicSyncList;

};
//}//_SyncManager
#endif // SYNC_SERVICE_CAPABILITY_INFO_H
