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
 * @file    SyncManager_SyncStatusInfo.h
 * @brief   This is the header file for the SyncStatusInfo class.
 */

#ifndef SYNC_SERVICE_STATUS_INFO_H
#define SYNC_SERVICE_STATUS_INFO_H

#include <stdio.h>
#include <vector>
#include <string>


/*namespace _SyncManager
{
*/
using namespace std;

class SyncStatusInfo
{
public:
	SyncStatusInfo(int capabilityId);

	SyncStatusInfo(SyncStatusInfo &other);

	SyncStatusInfo& operator =(SyncStatusInfo& other);

	SyncStatusInfo(string statusInfo);

	void SetPeriodicSyncTime(unsigned int index, long long when);

	long long GetPeriodicSyncTime(unsigned int index);

	void RemovePeriodicSyncTime(unsigned int index);

	string GetStatusInfoString(void);

public:
	int capabilityId;

private:
	void EnsurePeriodicSyncTimeSize(unsigned int index);

	// Warning: It is up to the external caller to ensure there are
	// no race conditions when accessing this list
private:
	vector<long long> __periodicSyncTimes;
};
//}//_SyncManager
#endif
