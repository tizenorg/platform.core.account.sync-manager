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
 * @file	SyncManager_SyncAdapterAggregator.h
 * @brief	This is the header file for the SyncManager class.
 */

#ifndef SYNC_SERVICE_SYNC_JOBS_AGGREGATOR_H
#define SYNC_SERVICE_SYNC_JOBS_AGGREGATOR_H

#include <iostream>
#include <list>
#include <bundle.h>
#include <stdio.h>
#include "SyncManager_SyncService.h"
#include "SyncManager_SyncWorker.h"
#include "SyncManager_Singleton.h"
#include "SyncManager_CurrentSyncJobQueue.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/

class RepositoryEngine;
class SyncJobsInfo;

using namespace std;

class SyncJobsAggregator {
public:
	void AddSyncJob(const char* pPackageId, const char* pSyncJobName, ISyncJob* pSyncJob);

	int RemoveSyncJob(const char* pPackageId, int jobId);

	int RemoveSyncJob(const char* pPackageId, const char* pSyncJobName);

	bool HasSyncJob(const char* pPackageId, const char* pSyncJobName);

	int GetSyncJobId(const char* pPackageId, const char* pSyncJobName);

	int GenerateSyncJobId(const char* pPackageId);

	ISyncJob* GetSyncJob(const char* pPackageId, const char* pSyncJobName);

	ISyncJob* GetSyncJob(const char* pPackageId, int syncJobId);

	vector < int > GetSyncJobIDList(const char* pPackageId);

	SyncJobsInfo* GetSyncJobsInfo(const char* pPackageId);

	void HandlePackageUninstalled(const char* pPackageId);

	map < string, SyncJobsInfo * > &GetAllSyncJobs();

	void SetMinPeriod(int min_period);

	void SetLimitTime(int min_period);

	int GetMinPeriod(void);

	int GetLimitTime(void);

protected:
	SyncJobsAggregator(void);

	~SyncJobsAggregator(void);

	friend class Singleton < SyncManager > ;

/*
class SyncJobsInfo
{
public:
	SyncJobsInfo(string __packageId);

private:
	SyncJobsInfo(const SyncJobsInfo&);

	const SyncJobsInfo& operator=(const SyncJobsInfo&);

public:
	map<string, SyncJob*> __syncJobs;
	int __nextJobId;
};
*/

private:
	SyncJobsAggregator(const SyncJobsAggregator&);

	const SyncJobsAggregator &operator = (const SyncJobsAggregator&);

private:
	map < string, SyncJobsInfo * > __syncJobsContainer;

	int min_period;
	int limit_time;

	friend class SyncManager;
	friend class RepositoryEngine;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_JOBS_AGGREGATOR_H */
