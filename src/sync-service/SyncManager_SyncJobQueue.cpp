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
 * @file	SyncManager_SyncJobQueue.cpp
 * @brief	This is the implementation file for the SyncJobQueue class.
 */

#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <dlog.h>
#include <sync-error.h>
#include "sync-log.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncJobQueue.h"

/*namespace _SyncManager
{*/

#ifndef MIN
#define MIN(a, b) a < b ? a : b
#endif

extern "C"
{


SyncJobQueue::SyncJobQueue(void)
{
	//Empty
}


SyncJobQueue::SyncJobQueue(RepositoryEngine* pSyncRepositoryEngine)
{
	__pSyncRepositoryEngine = pSyncRepositoryEngine;
}


SyncJobQueue::~SyncJobQueue(void)
{
	//Empty
}


map<const string, SyncJob*>
SyncJobQueue::GetSyncJobQueue(void) const
{
	return __syncJobsList;
}

//TODO Uncomment the below code when enabling pending job logic
/*
void SyncJobQueue::AddPendingJobs(string appId)
{
	LOG_LOGD("SyncJobQueue::addPendingJobs() Starts, appId[%s]", appId.c_str());

	list<PendingJob*> pendingJobsList = __pSyncRepositoryEngine->GetPendingJobs();
	for (int i = 0; i < pendingJobsList.size(); i++)
	{
		PendingJob* pPendingJob = pendingJobsList[i];
		if (appId.compare(pPendingJob->appId) != 0)
		{
			continue;
		}

		backOff* pBackOff = __pSyncRepositoryEngine->GetBackoff(pPendingJob->account, pPendingJob->capability);
		SyncJob* pSyncJob = new SyncJob(pPendingJob->appId, pPendingJob->account, pPendingJob->capability,
										pPendingJob->pExtras, pPendingJob->reason, pPendingJob->syncSource,
										0, 0, pBackOff != NULL ? pBackOff->time,
										__pSyncRepositoryEngine->GetDelayUntilTime(pPendingJob->account, pPendingJob->capability),
										false);
		if (pSyncJob == NULL)
		{
			LOG_LOGD("Failed to construct SyncJob");
			continue;
		}
		pSyncJob->isExpedited = pPendingJob->isExpedited;
		pSyncJob->pPendingJob = pPendingJob;
		int err = AddSyncJob(pSyncJob, pPendingJob);
		if (err != SYNC_ERROR_NONE)
		{
			LOG_LOGD("Failed to add pending job to sync jobs list");
		}
	}
	LOG_LOGD("SyncJobQueue::addPendingJobs() ends");
}*/


int
SyncJobQueue::AddSyncJob(SyncJob job)
{
	return AddSyncJob(job, NULL);
}


int
SyncJobQueue::AddSyncJob(SyncJob pSyncJob, PendingJob* pPendingJob)
{
	LOG_LOGD("Add to SyncJob Queue");
	LOG_LOGD("SyncJob Queue size, before = %d", __syncJobsList.size());

	/*
	 * If there is an existing operation with same key, check if the new one should run sooner
	 * Replace the run interval of existing with this new one
	 */
	string jobKey;

	if (pSyncJob.key.empty())
	{
		LOG_LOGD("Invalid key");
		return SYNC_ERROR_IO_ERROR;
	}

	stringstream ss;
	ss << pSyncJob.key;
	jobKey = ss.str();

	//If a job with same key already exists but this one has smaller runtime,
	// then replace the runTime of the existing sync job.
	map<const string, SyncJob*>::iterator it;
	it = __syncJobsList.find(jobKey);

	if (it != __syncJobsList.end())
	{
		SyncJob* pOtherJob = it->second;
		if (pSyncJob.Compare(pOtherJob) > 0)
		{
			return SYNC_ERROR_ALREADY_IN_PROGRESS;
		}

		pOtherJob->isExpedited = pSyncJob.isExpedited;
		pOtherJob->latestRunTime = MIN(pSyncJob.latestRunTime, pOtherJob->latestRunTime);
		pOtherJob->flexTime = pSyncJob.flexTime;

		LOG_LOGD("SyncJob Queue size, After = %d", __syncJobsList.size());
		return SYNC_ERROR_NONE;
	}

	//TODO Uncomment the below code when enabling pending job logic
	/*
	pSyncJob->pPendingJob = pPendingJob;

	if (pSyncJob->pPendingJob == NULL)
	{
		pPendingJob = new PendingJob(pSyncJob->appId, pSyncJob->account, pSyncJob->reason,
													pSyncJob->syncSource, pSyncJob->capability, pSyncJob->pExtras, pSyncJob->isExpedited);
		if (pPendingJob == NULL)
		{
			LOG_LOGD("Failed to create pending job instance.");
			return SYNC_ERROR_OUT_OF_MEMORY;
		}
		pPendingJob = __pSyncRepositoryEngine->InsertIntoPending(pPendingJob);
		if (pPendingJob == NULL)
		{
			LOG_LOGD("Failed to insert into pending jobs list");
			return SYNC_ERROR_INVALID_OPERATION;
		}
		pSyncJob->pPendingJob = pPendingJob;
	}
*/

	SyncJob* pJob = new (std::nothrow) SyncJob(pSyncJob);
	if (pJob == NULL)
	{
		LOG_LOGD("Failed to construct SyncJob");
		return SYNC_ERROR_OUT_OF_MEMORY;
	}

	pair<map<const string, SyncJob*>::iterator,bool> ret;
	ret = __syncJobsList.insert(pair<const string, SyncJob*>(jobKey, pJob));
	if (ret.second == false)
	{
		LOG_LOGD("Key already exits");
		return SYNC_ERROR_ALREADY_IN_PROGRESS;
	}

	LOG_LOGD("SyncJob Queue size, After = %d", __syncJobsList.size());

	return SYNC_ERROR_NONE;
}


void
SyncJobQueue::RemoveSyncJobsForApp(string appId)
{
	LOG_LOGD("Removing job from sync queue for app id[%s]", appId.c_str());
	map<const string, SyncJob*>::iterator it;
	for (it = __syncJobsList.begin(); it != __syncJobsList.end(); it++)
	{
		SyncJob* pSyncJob = it->second;
		if (pSyncJob && (pSyncJob->appId.compare(appId) == 0))
		{
			LOG_LOGD("Removing job[key:%s] from sync queue", pSyncJob->key.c_str());
			RemoveSyncJob(pSyncJob->key);
		}
	}
}


int
SyncJobQueue::RemoveSyncJob(string key)
{
	LOG_LOGD("Removing job and its corresponding pending operation, key[%s]", key.c_str());
	map<const string, SyncJob*>::iterator it = __syncJobsList.find(key);
	if (it == __syncJobsList.end())
	{
		LOG_LOGD("Can not find the key[%s] to remove", key.c_str());
		return SYNC_ERROR_INVALID_OPERATION;
	}

	SyncJob* pJob = it->second;
	if (__syncJobsList.erase(key) != 1)
	{
		LOG_LOGD("Failed to remove sync job from sync jobs list, key[%s]", key.c_str());
		return SYNC_ERROR_INVALID_OPERATION;
	}

	//TODO Uncomment the below code when enabling pending job logic
	/*if (!__pSyncRepositoryEngine->DeleteFromPending(pJob->pPendingJob))
	{
		LOG_LOGD("Unable to find pending job entry for syncjob, key[%s]", pSyncJob->key.c_str());
	}*/

	delete pJob;
	pJob = NULL;
	LOG_LOGD("Removed from __syncJobsList, size = %d", __syncJobsList.size());
	return SYNC_ERROR_NONE;
}


void
SyncJobQueue::OnBackoffChanged(account_h account, const string capability, long backoff)
{
	map<const string, SyncJob*>:: iterator it;
	for (it = __syncJobsList.begin(); it != __syncJobsList.end(); it++)
	{
		SyncJob* pJob = it->second;
		if (SyncManager::GetInstance()->AreAccountsEqual(pJob->account, account)  && !(pJob->capability.compare(capability)))
		{
			pJob->backoff = backoff;
			pJob->UpdateEffectiveRunTime();
		}
	}
}


void
SyncJobQueue::OnDelayUntilTimeChanged(account_h account, const string capability, long delayUntil)
{
	map<const string, SyncJob*>:: iterator it;
	for (it = __syncJobsList.begin(); it != __syncJobsList.end(); it++)
	{
		SyncJob* pJob = it->second;
		if (SyncManager::GetInstance()->AreAccountsEqual(pJob->account, account) && !(pJob->capability.compare(capability)))
		{
			pJob->delayUntil = delayUntil;
			pJob->UpdateEffectiveRunTime();
		}
	}
}


void
SyncJobQueue::RemoveSyncJob(string appId, account_h account, const string capability)
{
	map<const string, SyncJob*>:: iterator it;
	for (it = __syncJobsList.begin(); it != __syncJobsList.end();)
	{
		SyncJob* pSyncJob = it->second;

		if (account != NULL && !(SyncManager::GetInstance()->AreAccountsEqual(account, pSyncJob->account)))
		{
			it++;
			continue;
		}

		if (!(capability.empty()) && pSyncJob->capability.compare(capability))
		{
			it++;
			continue;
		}

		if (!(appId.empty()) && pSyncJob->appId.compare(appId))
		{
			it++;
			continue;
		}

		LOG_LOGD("Removing job and its corresponding pending operation, key[%s]", pSyncJob->key.c_str());

		//TODO Uncomment the below code when enabling pending job logic
		/*if (!__pSyncRepositoryEngine->DeleteFromPending(pSyncJob->pPendingJob))
		{
			LOG_LOGD("Unable to find pending job for syncjob, key[%s]", pSyncJob->key.c_str());
		}*/

		__syncJobsList.erase(it++);
		delete pSyncJob;
		pSyncJob = NULL;
		LOG_LOGD("Removed from syncJobsList, size = %d", __syncJobsList.size());
	}
}

}
//}//_SyncManager
