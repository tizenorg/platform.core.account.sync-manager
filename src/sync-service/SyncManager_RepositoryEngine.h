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
#include "SyncManager_CapabilityInfo.h"
#include "SyncManager_SyncStatusInfo.h"
#include "SyncManager_SyncJob.h"
#include "SyncManager_PeriodicSyncJob.h"



/*namespace _SyncManager
{
*/

using namespace std;


class SyncJobQueue;
class SyncJob;

class RepositoryEngine
{
	friend class CapabilityInfo;

public:
	static RepositoryEngine* GetInstance(void);

	~RepositoryEngine(void);

	void OnBooting();

	void SaveCurrentState(void);

public:
	static const long NOT_IN_BACKOFF_MODE;

private:

	RepositoryEngine(void);

	RepositoryEngine(const RepositoryEngine&);

	const RepositoryEngine& operator=(const RepositoryEngine&);

	void ReadSyncJobsData(void);

	void WriteSyncJobsData(void);

	void ReadSyncAdapters(void);

	void WriteSyncAdapters(void);

	void ParseCapabilities(xmlNodePtr cur);

	void ParsePeriodicSyncs(xmlNodePtr cur, xmlChar* pCapability);

	void ParseExtras(xmlNodePtr cur, bundle* pExtra);

	void ParseSyncJobsN(xmlNodePtr cur, xmlChar* pPackage);

private:
	pthread_mutex_t __capabilityInfoMutex;

	vector<PeriodicSyncJob*> __pendingJobList;			// Pending periodic job list to be scheduled
	//map<string, DataSyncJob*> __pendingDataSyncJobList;				// Data sync job list to be scheduled

	map<string, CapabilityInfo*> __capabilities;
	map<string, map<string, SyncJob*> > __Aggr;				// Data sync job list to be scheduled
	map<int, SyncStatusInfo*> __syncStatus;

	static RepositoryEngine* __pInstance;

	static const long DEFAULT_PERIOD_SEC;
	static const double DEFAULT_FLEX_PERCENT;
	static const long DEFAULT_MIN_FLEX_ALLOWED_SEC;
};
//}//_SyncManager
#endif // SYNC_SERVICE_REPOSITORY_ENGINE_H
