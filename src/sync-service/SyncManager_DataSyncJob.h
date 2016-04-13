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
 * @file	SyncManager_DataSyncJob.h
 * @brief	This is the header file for the DataSyncJob class.
 */

#ifndef SYNC_SERVICE_DATA_SYNC_JOB_H
#define SYNC_SERVICE_DATA_SYNC_JOB_H

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

class DataSyncJob
			: public SyncJob {
public:
	~DataSyncJob(void);

	DataSyncJob(const string appId, const string syncJobName, int accountId, bundle *pUserData, int syncOption, int syncJobId, SyncType type, string capability);

	DataSyncJob(const DataSyncJob&);

	DataSyncJob &operator = (const DataSyncJob&);

	void Reset(int accountId, bundle* pUserData, int syncOption, string capability);

	virtual SyncType GetSyncType() {
		return __syncType;
	}

public:
	string __capability;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_PERIODIC_SYNC_H */
