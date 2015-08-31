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
 * @file	SyncManager_RepositoryEngine.h
 * @brief	This is the header file for the RepositoryEngine class.
 */

#ifndef SYNC_SERVICE_REPOSITORY_ENGINE_H
#define SYNC_SERVICE_REPOSITORY_ENGINE_H

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <bundle.h>
#include <bundle_internal.h>
#include <account.h>
#include <pthread.h>
#include <vector>
#include <list>
#include <map>
#include "SyncManager_DayStats.h"
#include "SyncManager_HistoryItem.h"
#include "SyncManager_CapabilityInfo.h"
#include "SyncManager_SyncStatusInfo.h"
#include "SyncManager_PendingJob.h"
#include "SyncManager_PeriodicSyncJob.h"
#include "SyncManager_SyncJob.h"


/*namespace _SyncManager
{
*/

using namespace std;

typedef struct backOff
{
	long time;
	long delay;
}backOff;

class SyncJobQueue;
class SyncJob;

class RepositoryEngine
{
	friend class CapabilityInfo;

public:

	class AccountInfo
	{
	public:
		account_h account;
		string appId;
		map<string, CapabilityInfo*> capabilities;
		AccountInfo(account_h account, string appId) { this->account = account; this->appId = appId; }
	};

public:
	static RepositoryEngine* GetInstance(void);

	~RepositoryEngine(void);

	long GetRandomOffsetInsec(void) { return __randomOffsetInSec; }

	backOff* GetBackoffN(account_h account, const string capability);

	long GetDelayUntilTime(account_h account, const string capability);

	//Uncomment below code when implementing pending job logic
	//list<PendingJob*> GetPendingJobs(void);

	bool DeleteFromPending(PendingJob* pPendingJob);

	PendingJob* InsertIntoPending(PendingJob* pPendingJob);

	long CalculateDefaultFlexTime(long period);

	void AddPeriodicSyncJob(string appId, PeriodicSyncJob* pJob, bool accountLess);

	void RemovePeriodicSyncJob(string appId, PeriodicSyncJob* pJob);

	vector<pair<CapabilityInfo*, SyncStatusInfo*> > GetCopyOfAllCapabilityAndSyncStatusN(void);

	bool GetSyncAutomatically(account_h account, string capability);

	int GetSyncable(account_h account, string capability);

	void SetSyncable(string appId, account_h account, string capability, int syncable);

	void SetPeriodicSyncTime(int capabilityId, PeriodicSyncJob* pJob , long long when);

	void SetBackoffValue(string appId, account_h account, string providerName, long nextSyncTime, long nextDelay);

	void RemoveAllBackoffValuesLocked(SyncJobQueue* pQueue);

	void SaveCurrentState(void);

	void CleanUp(string appId);

public:
	static const long NOT_IN_BACKOFF_MODE;

private:

	RepositoryEngine(void);

	RepositoryEngine(const RepositoryEngine&);

	const RepositoryEngine& operator=(const RepositoryEngine&);

	void ReadAccountData(void);

	void WriteAccountData(void);

	void ReadStatusData(void);

	void WriteStatusData(void);

	void ReadSyncJobsData(void);

	void WriteSyncJobsData(void);

	void ReadSyncAdapters(void);

	void WriteSyncAdapters(void);

	CapabilityInfo* ParseCapabilities(xmlNodePtr cur);

	void ParsePeriodicSyncs(xmlNodePtr cur, CapabilityInfo* pCapabilityInfo);

	void ParseExtras(xmlNodePtr cur, bundle* pExtra);

	SyncJob* ParseSyncJobsN(xmlNodePtr cur);

	CapabilityInfo* GetOrCreateCapabilityLocked(string appId, account_h account, const string capability, int id, bool toWriteToXml);

	CapabilityInfo* GetCapabilityLocked(account_h account, const string capability, const char* pTag);

	SyncStatusInfo* GetOrCreateSyncStatusLocked(int capabilityId);

	pair<CapabilityInfo*, SyncStatusInfo*> CreateCopyOfCapabilityAndSyncStatusPairN(CapabilityInfo* pCapabilityInfo);

private:
	pthread_mutex_t __capabilityInfoMutex;
	map<int, AccountInfo*> __accounts;
	//list<PendingJob*> __pendingJobList;
	map<int, SyncStatusInfo*> __syncStatus;
	int __numPendingFinished;
	int __nextCapabilityId;
	map<int, CapabilityInfo*> __capabilities;
	long __randomOffsetInSec;
	int PENDING_FINISH_TO_WRITE;

	static RepositoryEngine* __pInstance;

	static const long DEFAULT_PERIOD_SEC;
	static const double DEFAULT_FLEX_PERCENT;
	static const long DEFAULT_MIN_FLEX_ALLOWED_SEC;
};
//}//_SyncManager
#endif // SYNC_SERVICE_REPOSITORY_ENGINE_H
