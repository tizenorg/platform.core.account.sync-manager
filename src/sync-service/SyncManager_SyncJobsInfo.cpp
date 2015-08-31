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
#include "SyncManager_SyncJobsInfo.h"
#include "SyncManager_SyncManager.h"


/*namespace _SyncManager
{*/


SyncJobsInfo::~SyncJobsInfo(void)
{
}


SyncJobsInfo::SyncJobsInfo(string packageId)
			: __packageId(packageId)
{
	for (int i = 0; i < SYNC_JOB_LIMIT; i++)
	{
		__syncJobId[i] = false;
	}

}


int
SyncJobsInfo::AddSyncJob(string syncJobName, ISyncJob* pSyncJob)
{
	__syncJobsList.insert(make_pair(syncJobName, pSyncJob));
	__syncIdList.insert(make_pair(pSyncJob->GetSyncJobId(), pSyncJob));
	__syncJobId[pSyncJob->GetSyncJobId()] = true;
}


ISyncJob*
SyncJobsInfo::GetSyncJob(string syncJobName)
{
	map<string, ISyncJob*>::iterator itr = __syncJobsList.find(syncJobName);
	if (itr != __syncJobsList.end())
	{
		return itr->second;
	}
	return NULL;
}


ISyncJob*
SyncJobsInfo::GetSyncJob(int syncJobId)
{
	map<int, ISyncJob*>::iterator itr = __syncIdList.find(syncJobId);
	if (itr != __syncIdList.end())
	{
		LOG_LOGD("Found sync job for id [%d]", syncJobId);
		return itr->second;
	}

	return NULL;
}


int
SyncJobsInfo::RemoveSyncJob(int syncJobId)
{
	map<int, ISyncJob*>::iterator itr = __syncIdList.find(syncJobId);
	if (itr != __syncIdList.end())
	{
		SyncJob* syncJob = dynamic_cast< SyncJob* > (itr->second);
		if (syncJob != NULL)
		{
			RemoveSyncJob(syncJob->__syncJobName);
		}
	}

	return 0;
}


int
SyncJobsInfo::RemoveSyncJob(string syncJobname)
{
	map<string, ISyncJob*>::iterator itr = __syncJobsList.find(syncJobname);
	if (itr != __syncJobsList.end())
	{
		ISyncJob* pSyncJob = itr->second;
		int syncJobId = pSyncJob->GetSyncJobId();
		__syncJobId[syncJobId] = false;
		LOG_LOGD("Removing job name [%s] id [%d] from package [%s]", syncJobname.c_str(), syncJobId, __packageId.c_str());

		delete pSyncJob;
		__syncJobsList.erase(itr);
		__syncIdList.erase(syncJobId);
	}
	else
	{
		LOG_LOGD("Sync job name doesnt exists in package [%s] for job name [%s]", __packageId.c_str(), syncJobname.c_str());
	}
	return 0;
}


int
SyncJobsInfo::GetNextSyncJobId()
{
	int value = -1;
	int idx;

	for (idx = 1; idx <= SYNC_JOB_LIMIT; idx++)
	{
		if (!__syncJobId[idx])
		{
			value = idx;
			break;
		}
	}

	if (idx == SYNC_JOB_LIMIT)
	{
		value = SYNC_JOB_LIMIT + 1;
	}

	return value;
}


vector <int>
SyncJobsInfo::GetSyncJobIdList()
{
	vector<int> idList;
	int idx;

	for (idx = 0; idx < SYNC_JOB_LIMIT; idx++)
	{
		if (__syncJobId[idx])
		{
			idList.push_back(idx);
		}
	}
	return idList;
}


map<int, ISyncJob*>&
SyncJobsInfo::GetAllSyncJobs()
{
	return __syncIdList;
}


int
SyncJobsInfo::GetSyncJobsCount()
{
	return __syncJobsList.size();
}

//}//_SyncManager
