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
 * @file	SyncManager_PeriodicSyncJob.h
 * @brief	This is the header file for the PeriodicSyncJob class.
 */

#ifndef SYNC_SERVICE_PERIODIC_SYNC_H
#define SYNC_SERVICE_PERIODIC_SYNC_H

#include <string>
#include <vector>
#include <bundle.h>
#include <bundle_internal.h>
#include <account.h>
#include "SyncManager_SyncJob.h"

/*namespace _SyncManager
{
*/

using namespace std;

class PeriodicSyncJob
			: public SyncJob
{
public:
	~PeriodicSyncJob(void);

	PeriodicSyncJob(const string appId, const string syncJobName, int accountId, bundle *pUserData, int syncOption, int syncJobId, SyncType type, long frequency);

	PeriodicSyncJob(const PeriodicSyncJob&);

	PeriodicSyncJob &operator = (const PeriodicSyncJob&);

	void Reset(int accountId, bundle* pUserData, int syncOption, long frequency);

	bool operator == (const PeriodicSyncJob&);

	bool IsExtraEqual(PeriodicSyncJob* pJob);

	virtual SyncType GetSyncType() {
		return __syncType;
	}

public:
	long __period;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_PERIODIC_SYNC_H */
