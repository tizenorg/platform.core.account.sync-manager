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
 * @file	SyncManager_SyncAdapterAggregator.cpp
 * @brief	This is the header file for the SyncManager class.
 */


#include <iostream>
#include <list>
#include <bundle.h>
#include <stdio.h>
#include "SyncManager_SyncJobsAggregator.h"
#include "SyncManager_SyncJobsInfo.h"
#include "SyncManager_SyncWorker.h"
#include "SyncManager_Singleton.h"
#include "SyncManager_CurrentSyncJobQueue.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/


SyncJobsAggregator::SyncJobsAggregator(void) {
}


/* LCOV_EXCL_START */
SyncJobsAggregator::~SyncJobsAggregator(void) {
}


/*
SyncJob*
SyncJobsAggregator::GetOrCreateSyncJob(const char* pPackageId, const char* pSyncJobName, SyncJob* pSyncJob)
{
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end())
	{
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		pPackageSyncJobsInfo->Add(pSyncJobName, pSyncJob);
	}
}
*/


bool
SyncJobsAggregator::HasSyncJob(const char* pPackageId, const char* pSyncJobName) {
	ISyncJob* pSyncJob = GetSyncJob(pPackageId, pSyncJobName);
	return pSyncJob != NULL;
}
/* LCOV_EXCL_STOP */


int
SyncJobsAggregator::GenerateSyncJobId(const char* pPackageId) {
	int id = -1;
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		id = pPackageSyncJobsInfo->GetNextSyncJobId();
	} else {
		LOG_LOGD("First request for the package [%s]", pPackageId);

		SyncJobsInfo* pPackageSyncJobsInfo = new (std::nothrow) SyncJobsInfo(pPackageId);
		id = pPackageSyncJobsInfo->GetNextSyncJobId();
		__syncJobsContainer.insert(make_pair(pPackageId, pPackageSyncJobsInfo));
	}

	return id;
}


void
SyncJobsAggregator::AddSyncJob(const char* pPackageId, const char* pSyncJobName, ISyncJob* pSyncJob) {
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		LOG_LOGD("Sync Jobs info found for package %s", pPackageId);
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		pPackageSyncJobsInfo->AddSyncJob(pSyncJobName, pSyncJob);
	} else {
		/* LCOV_EXCL_START */
		LOG_LOGD("Creating new Sync Jobs info handle for package %s", pPackageId);
		SyncJobsInfo* pPackageSyncJobsInfo = new (std::nothrow) SyncJobsInfo(pPackageId);
		pPackageSyncJobsInfo->AddSyncJob(pSyncJobName, pSyncJob);
		__syncJobsContainer.insert(make_pair(pPackageId, pPackageSyncJobsInfo));
		/* LCOV_EXCL_STOP */
	}
}


int
SyncJobsAggregator::RemoveSyncJob(const char* pPackageId, int syncJobId) {
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		int ret = pPackageSyncJobsInfo->RemoveSyncJob(syncJobId);
		if (pPackageSyncJobsInfo->GetSyncJobsCount() == 0) {
			delete pPackageSyncJobsInfo;
			__syncJobsContainer.erase(itr);
		}
		return ret;
	}

	return -1;
}


/* LCOV_EXCL_START */
int
SyncJobsAggregator::RemoveSyncJob(const char* pPackageId, const char* pSyncJobName) {
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		int ret = pPackageSyncJobsInfo->RemoveSyncJob(pSyncJobName);
		if (pPackageSyncJobsInfo->GetSyncJobsCount() == 0) {
			delete pPackageSyncJobsInfo;
			__syncJobsContainer.erase(itr);
		}
		return ret;
	} else {
		LOG_LOGD("Sync jobs for package %s are not found", pPackageId);
	}

	return -1;
}


int
SyncJobsAggregator::GetSyncJobId(const char* pPackageId, const char* pSyncJobName) {
	ISyncJob* pSyncJob = GetSyncJob(pPackageId, pSyncJobName);
	int id = (pSyncJob == NULL) ? -1 : pSyncJob->GetSyncJobId();

	return id;
}
/* LCOV_EXCL_STOP */


ISyncJob*
SyncJobsAggregator::GetSyncJob(const char* pPackageId, const char* pSyncJobName) {
	ISyncJob* pSyncJob = NULL;
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		pSyncJob = pPackageSyncJobsInfo->GetSyncJob(pSyncJobName);
	}

	return pSyncJob;
}


/* LCOV_EXCL_START */
void
SyncJobsAggregator::HandlePackageUninstalled(const char* pPackageId) {
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		pPackageSyncJobsInfo->RemoveAllSyncJobs();
		__syncJobsContainer.erase(pPackageId);
	} else {
		LOG_LOGD("Sync jobs for package %s are not found", pPackageId);
	}
}
/* LCOV_EXCL_STOP */


ISyncJob*
SyncJobsAggregator::GetSyncJob(const char* pPackageId, int syncJobId) {
	ISyncJob* pSyncJob = NULL;
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		pSyncJob = pPackageSyncJobsInfo->GetSyncJob(syncJobId);
	} else {
		LOG_LOGD("Sync jobs for package %s are not found", pPackageId);	/* LCOV_EXCL_LINE */
	}
	return pSyncJob;
}


/* LCOV_EXCL_START */
vector< int >
SyncJobsAggregator::GetSyncJobIDList(const char* pPackageId) {
	vector<int> list;
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		SyncJobsInfo* pPackageSyncJobsInfo = itr->second;
		list = pPackageSyncJobsInfo->GetSyncJobIdList();
	}
	return list;
}
/* LCOV_EXCL_STOP */


SyncJobsInfo*
SyncJobsAggregator::GetSyncJobsInfo(const char* pPackageId) {
	map<string, SyncJobsInfo*>::iterator itr = __syncJobsContainer.find(pPackageId);
	if (itr != __syncJobsContainer.end()) {
		return itr->second;
	}

	return NULL;
}


map<string, SyncJobsInfo*>&
SyncJobsAggregator::GetAllSyncJobs() {
	return __syncJobsContainer;
}

//}//_SyncManager
