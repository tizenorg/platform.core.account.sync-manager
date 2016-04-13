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

#ifndef SYNC_SERVICE_SYNC_JOBS_INFO_H
#define SYNC_SERVICE_SYNC_JOBS_INFO_H

#include <string>
#include <vector>
#include <account.h>
#include "SyncManager_ISyncJob.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/

using namespace std;


class SyncJobsInfo {
public:
	~SyncJobsInfo(void);

	SyncJobsInfo(string packageId);

	ISyncJob* GetSyncJob(string syncJobName);

	ISyncJob* GetSyncJob(int syncJobId);

	int RemoveSyncJob(int syncJobId);

	int RemoveSyncJob(string syncJobName);

	int AddSyncJob(string syncJobName, ISyncJob* pSyncJob);

	int GetNextSyncJobId();

	int GetSyncJobsCount();

	vector < int > GetSyncJobIdList();

	map < int, ISyncJob * > &GetAllSyncJobs();

	void RemoveAllSyncJobs();

private:
	SyncJobsInfo(const SyncJobsInfo&);

	const SyncJobsInfo &operator = (const SyncJobsInfo&);

public:
	map < string, ISyncJob * > __syncJobsList;
	map < int, ISyncJob * > __syncIdList;
	string __packageId;
	bool __syncJobId[SYNC_JOB_LIMIT + 1];
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_JOBS_INFO_H */
