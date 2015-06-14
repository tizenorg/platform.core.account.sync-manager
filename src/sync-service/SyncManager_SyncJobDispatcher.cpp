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
 * @file	SyncManager_SyncJobDispatcher.cpp
 * @brief	This is the implementation file for the SyncJobDispatcher class.
 */

#include <sys/time.h>
#include <climits>
#include <alarm.h>
#include <glib.h>
#include <unistd.h>
#include <device/power.h>
#include "sync-error.h"
#include "sync-log.h"
#include <aul.h>
#include "SyncManager_SyncJobDispatcher.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_SyncJob.h"

#ifndef MAX
#define MAX(a, b) a>b?a:b
#endif
#ifndef MIN
#define MIN(a, b) a < b ? a : b
#endif

long MAX_TIME_PER_SYNC = 5*60*1000; //5 minutes
long MAX_SIMULTANEOUS_INITIALIZATION_SYNCS = 2;
long MAX_SIMULTANEOUS_REGULAR_SYNCS = 10;
long MAX_TIMEOUT_VAL = 2*60*60*1000; // 2 hours
long MIN_TIMEOUT_VAL = 30*1000; // 30 seconds

long long scheduledTimeoutTime = -1;
alarm_id_t alarm_id = 0;

using namespace std;
/*namespace _SyncManager
{*/

SyncJobDispatcher::SyncJobDispatcher(void)
{
}


SyncJobDispatcher::~SyncJobDispatcher(void)
{
}


int
SyncJobDispatcher::DispatchSyncJob(SyncJob syncJob)
{
	LOG_LOGD("Dispatching sync job %s, %s", syncJob.appId.c_str(), syncJob.capability.c_str());

	if( syncJob.account == NULL)
	{
		SyncService::GetInstance()->TriggerStartSync(syncJob.appId.c_str(), -1, syncJob.pExtras, NULL);
	}
	else
	{
		int accId;
		int ret = account_get_account_id(syncJob.account, &accId);
		if (ret == ACCOUNT_ERROR_NONE)
		{
			SyncService::GetInstance()->TriggerStartSync(syncJob.appId.c_str(), accId, syncJob.pExtras, syncJob.capability.c_str());
		}
	}

	if (SyncManager::GetInstance()->__pCurrentSyncJobQueue)
	{
		pthread_mutex_lock(&(SyncManager::GetInstance()->__currJobQueueMutex));
		LOG_LOGD("Add to Active Sync queue");
		SyncManager::GetInstance()->__pCurrentSyncJobQueue->AddSyncJobToCurrentSyncQueue(syncJob);
		pthread_mutex_unlock(&(SyncManager::GetInstance()->__currJobQueueMutex));
	}

	return SYNC_ERROR_NONE;
}


void
SyncJobDispatcher::HandleJobCompletedOrCancelledLocked(SyncStatus res, SyncJob *pJob)
{
	LOG_LOGD("Starts");

	switch (res)
	{
	case SYNC_STATUS_SUCCESS:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_SUCCESS");
		SyncManager::GetInstance()->ClearBackoffValue(pJob);
		break;

	case SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS");
		SyncManager::GetInstance()->TryToRescheduleJob(res, pJob);
		break;

	case SYNC_STATUS_FAILURE:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_FAILURE");
		SyncManager::GetInstance()->IncreaseBackoffValue(pJob);
		SyncManager::GetInstance()->TryToRescheduleJob(res, pJob);
		break;

	case SYNC_STATUS_CANCELLED:
		LOG_LOGD("Handle Sync event : SYNC_STATUS_CANCELLED");
		SyncService::GetInstance()->TriggerStopSync(pJob->appId.c_str(), pJob->account, pJob->capability.c_str());
		break;

	default:
		break;
	}
}

void
SyncJobDispatcher::OnEventReceived(Message msg)
{
	LOG_LOGD("0. Sync Job dispatcher starts");
	if (!SyncManager::GetInstance()->__isSyncPermitted)
	{
		LOG_LOGD("Sync not permitted now");
		if (alarm_id > 0)
		{
			LOG_LOGD("cancelling active alarm");
			alarmmgr_remove_alarm(alarm_id);
		}
		return;
	}

	long long earliestFuturePollTime = LLONG_MAX;
	long long nextPendingSyncTime = LLONG_MAX;

	LOG_LOGD("1. Schedule periodic sync job to Main queue ");
	earliestFuturePollTime = SchedulePeriodicSyncJobs();
	if (earliestFuturePollTime !=  LLONG_MAX)
	{
		LOG_LOGD("Earliest future polling time among periodic sync jobs %lld s", earliestFuturePollTime / 1000);
	}

	int ret = device_power_request_lock(POWER_LOCK_DISPLAY, 1000);// power lock for one minute: to keep cpu awake
	if (ret != DEVICE_ERROR_NONE)
	{
		return;
	}
	switch (msg.type)
	{
		case SYNC_CANCEL:
			{
				LOG_LOGD("2. Handle Event : SYNC_CANCEL");
				HandleJobCompletedOrCancelledLocked(SYNC_STATUS_CANCELLED, msg.pSyncJob);
				LOG_LOGD("3. Start next Syncjob from main queue");
				nextPendingSyncTime = TryStartingNextJobLocked();
			}
			break;

		case SYNC_FINISHED:
			{
				LOG_LOGD("2. Handle Event : SYNC_FINISHED");
				HandleJobCompletedOrCancelledLocked(msg.res, msg.pSyncJob);
				LOG_LOGD("3. Start next Sync job from main queue");
				nextPendingSyncTime = TryStartingNextJobLocked();
			}
			break;

		case SYNC_CHECK_ALARM:
			{
				LOG_LOGD("2. Handle Event : SYNC_CHECK_ALARM");
				LOG_LOGD("3. Start next Sync job from main queue");
				nextPendingSyncTime = TryStartingNextJobLocked();
			}
			break;

		case SYNC_ALARM:
			{
				LOG_LOGD("2. Handle Event : SYNC_ALARM");
				LOG_LOGD("3. Start next Sync job from main queue");
				nextPendingSyncTime = TryStartingNextJobLocked();
			}
			break;
		default:
			break;
	};

	LOG_LOGD("4. Schedule an alarm");
	if (nextPendingSyncTime != LLONG_MAX)
	{
		LOG_LOGD("Next pending poll time among main queue sync jobs [%lld s]", nextPendingSyncTime / 1000);
	}

	long long newTimeoutTime = MIN(nextPendingSyncTime, earliestFuturePollTime);
	if (newTimeoutTime != LLONG_MAX)
	{
		if (scheduledTimeoutTime != -1)
		{
			LOG_LOGD("Scheduled alarm poll time [%lld s]", scheduledTimeoutTime / 1000);
		}

		if (scheduledTimeoutTime == -1 || newTimeoutTime < scheduledTimeoutTime)
		{
			scheduledTimeoutTime = newTimeoutTime;
			if (alarm_id > 0)
			{
				LOG_LOGD("Clearing previous alarm  id %ld", alarm_id);
				alarmmgr_remove_alarm(alarm_id);
			}
			LOG_LOGD("Effective poll time [%lld s]", scheduledTimeoutTime / 1000);
			long timeout = scheduledTimeoutTime / 1000;
//			int ret = alarmmgr_add_periodic_alarm_withcb(timeout, QUANTUMIZE, SyncJobDispatcher::OnAlarmExpired, NULL, &alarm_id);
//			LOG_LOGD("Alarm added for %ld s, id %ld, status %d", timeout, alarm_id, ret);
		}
	}

		/// long long elapsedTime = SyncManager::GetInstance()->GetElapsedTime();
		/// bool isAlarmActive = (scheduledTimeoutTime != -1) && (elapsedTime < scheduledTimeoutTime);
		/*if (toCancel)
		{
			scheduledTimeoutTime = -1;
			if (alarm_id > 0)
			{
				LOG_LOGD("cancelling alarm set earlier, id %ld", alarm_id);
				alarmmgr_remove_alarm(alarm_id);
			}
		}*/


	ret = device_power_release_lock(POWER_LOCK_DISPLAY);// power unlock
	if (ret != DEVICE_ERROR_NONE)
	{
		return;
	}

	LOG_LOGD("5. Sync Job dispatcher Ends");
}


int
SyncJobDispatcher::OnAlarmExpired(alarm_id_t alarm_id, void *user_param)
{
	LOG_LOGD("Alarm id %d", alarm_id);
	SyncManager::GetInstance()->SendSyncAlarmMessage();
	return true;
}


bool
sortFunc(const SyncJob* pJob1, const SyncJob* pJob2)
{
	if (pJob1->isExpedited != pJob2->isExpedited) {
		return pJob1->isExpedited  ? false : true;
	}
	long firstIntervalStart = MAX(pJob1->effectiveRunTime - pJob1->flexTime, 0);
	long secondIntervalStart = MAX(pJob2->effectiveRunTime - pJob1->flexTime, 0);
	if (firstIntervalStart > secondIntervalStart)
	{
		return true;
	}
	else
	{
		return false;
	}
}

long long
SyncJobDispatcher::TryStartingNextJobLocked()
{
	//TODO: remove this line for deviv, needed while testing on emulator
   // SyncManager::GetInstance()->__isWifiConnectionPresent = true;
	if (SyncManager::GetInstance()->__isWifiConnectionPresent == false && SyncManager::GetInstance()->__isSimDataConnectionPresent == false)
	{
		LOG_LOGD("No network available: Skipping sync");
		return LLONG_MAX;
	}

	if (!SyncManager::GetInstance()->__isSyncPermitted)
	{
		LOG_LOGD("Sync not permitted now: Skipping sync");
		return LLONG_MAX;
	}

	if (SyncManager::GetInstance()->__isUPSModeEnabled)
	{
		LOG_LOGD("UPS mode enabled: Skipping sync");
		return LLONG_MAX;
	}

	if (SyncManager::GetInstance()->__isStorageLow)
	{
		 LOG_LOGD("Storage Low: Skipping sync");
		 return LLONG_MAX;
	}

	//TODO: check if accounts are ready

	//TODO: maintain a variable to know if background data usage allowed

	long long now = SyncManager::GetInstance()->GetElapsedTime();

	long long nextReadyToRunTime = LLONG_MAX;

	pthread_mutex_lock(&(SyncManager::GetInstance()->__syncJobQueueMutex));
	map<const string, SyncJob*> jobQueue = SyncManager::GetInstance()->__pSyncJobQueue->GetSyncJobQueue();

	if (jobQueue.empty())
	{
		LOG_LOGD("SyncJob Queue is empty");
		pthread_mutex_unlock(&(SyncManager::GetInstance()->__syncJobQueueMutex));
		return LLONG_MAX;
	}

	list<SyncJob*> listJobs;

	map<const string, SyncJob*>::iterator mapIt;
	list<SyncJob*>::iterator it;

	for (mapIt = jobQueue.begin(); mapIt != jobQueue.end(); ++mapIt)
	{
	  SyncJob* pNewSyncJob = new SyncJob(*(mapIt->second));
	  if (pNewSyncJob == NULL)
	  {
		  LOG_LOGD("Failed to construct SyncJob");
		  continue;
	  }
	  listJobs.push_back(pNewSyncJob);
	}

	pthread_mutex_unlock(&(SyncManager::GetInstance()->__syncJobQueueMutex));

	for (it = listJobs.begin(); it != listJobs.end(); it++)
	{
		LOG_LOGD("Sync Job Info Job %s", (*it)->key.c_str());
		LOG_LOGD("Effective Run time %ld flexTime %ld", (*it)->effectiveRunTime, (*it)->flexTime);

		//TODO: for every operation validate for correspondng account existence in running accounts

		//TODO: check if job is syncable

		//TODO: check if application provider is not running then drop the request

		// If the next run time is in the future, even given the flexible scheduling,
		// return the time.
		if ((*it)->effectiveRunTime - (*it)->flexTime > now)
		{
			if (nextReadyToRunTime >(*it)->effectiveRunTime)
			{
				nextReadyToRunTime = now - (*it)->effectiveRunTime;
			}
			LOG_LOGD(" Dropping sync operation: Sync too far in future. %ld %ld ", nextReadyToRunTime, (*it)->effectiveRunTime);
			continue;
		}

		if (nextReadyToRunTime != LLONG_MAX)
		{
			LOG_LOGD("nextReadyToRunTime %lld", nextReadyToRunTime);
		}

		//TODO: check if operation is allowed on metered network else drop it

		//TODO:  skip the sync if it isn't manual, and auto sync or
		// background data usage is disabled or network is
		// disconnected for the target UID.
	}

	listJobs.sort(sortFunc);
	for (it = listJobs.begin(); it != listJobs.end(); it++)
	{
		SyncJob *pCandidate = (*it);

		bool isCandidateInitialized = pCandidate->IsInitialized();
		int nInit = 0;
		int nRegular = 0;
		CurrentSyncContext *conflict = NULL;
		CurrentSyncContext *longRunning = NULL;
		CurrentSyncContext *toReschedule = NULL;
		CurrentSyncContext *oldestNonExpeditedRegular = NULL;
		bool alreadyInProgress = false;

		pthread_mutex_lock(&(SyncManager::GetInstance()->__currJobQueueMutex));
		list<CurrentSyncContext*> listCurrentSyncs = SyncManager::GetInstance()->__pCurrentSyncJobQueue->GetOperations();
		pthread_mutex_unlock(&(SyncManager::GetInstance()->__currJobQueueMutex));

		list<CurrentSyncContext*>::iterator it2;
		for (it2 = listCurrentSyncs.begin(); it2 != listCurrentSyncs.end(); it2++)
		{
			CurrentSyncContext* pCurrJob = new (std::nothrow) CurrentSyncContext(*(*(it2)));
			if (pCurrJob == NULL)
			{
				LOG_LOGD("Failed to construct CurrentSyncContext");
				continue;
			}
			SyncJob* pActiveJob = pCurrJob->GetSyncJob();
			if (pActiveJob == NULL)
			{
				LOG_LOGD("pActiveJob == NULL, continue");
				delete pCurrJob;
				pCurrJob = NULL;
				continue;
			}
			if (!pActiveJob->appId.compare(pCandidate->appId))
			{
				LOG_LOGD("Sync adapter is already handling a sync job");
				alreadyInProgress = true;
				break;
			}
			if (pActiveJob->IsInitialized())
			{
				nInit++;
			}
			else
			{
				nRegular++;
				if (pActiveJob->IsExpedited() == false)
				{
					if (oldestNonExpeditedRegular == NULL
					|| (oldestNonExpeditedRegular->GetStartTime()
						> pCurrJob->GetStartTime()))
					{
						oldestNonExpeditedRegular = pCurrJob;
					}
				}
			}
			if (SyncManager::GetInstance()->AreAccountsEqual(pActiveJob->account, pCandidate->account)
					&& !strcmp(pActiveJob->capability.c_str(), pCandidate->capability.c_str())
					&& (!pActiveJob->isParallelSyncAllowed))
			{
				conflict = pCurrJob;
				// don't break out since we want to do a full count of the varieties
			}
			else
			{
				if (isCandidateInitialized == pActiveJob->IsInitialized()
						&& pCurrJob->GetStartTime() + MAX_TIME_PER_SYNC < now)
				{
					longRunning = pCurrJob;
					// don't break out since we want to do a full count of the varieties
				}
			}
		}

		bool roomAvailable = isCandidateInitialized
			? nInit < MAX_SIMULTANEOUS_INITIALIZATION_SYNCS
			: nRegular < MAX_SIMULTANEOUS_REGULAR_SYNCS;

		if (conflict != NULL)
		{
			if (isCandidateInitialized && !conflict->GetSyncJob()->IsInitialized()
					&& nInit < MAX_SIMULTANEOUS_INITIALIZATION_SYNCS)
			{
				toReschedule = conflict;
			}
			else if (pCandidate->isExpedited && !conflict->GetSyncJob()->IsExpedited()
					&& (isCandidateInitialized == conflict->GetSyncJob()->IsInitialized()))
			{
				toReschedule = conflict;
			}
			else
			{
				LOG_LOGD("conflict != NULL, continue");
				continue;
			}
		}
		else if (roomAvailable)
		{
			// dispatch candidate
		}
		else if (pCandidate->IsExpedited() && oldestNonExpeditedRegular != NULL && !isCandidateInitialized)
		{
			// We found an active, non-expedited regular sync. We also know that the
			// candidate doesn't conflict with this active sync since conflict
			// is null. Reschedule the active sync and start the candidate.
			toReschedule = oldestNonExpeditedRegular;
		}
		else if (longRunning != NULL && (isCandidateInitialized == longRunning->GetSyncJob()->IsInitialized()))
		{
			// We found an active, long-running sync. Reschedule the active
			// sync and start the candidate.
			toReschedule = longRunning;
		}
		else
		{
			// we were unable to find or make space to run this candidate, go on to
			// the next one
			LOG_LOGD("unable to find or make space, continue");
			continue;
		}

		if (toReschedule != NULL)
		{
			SyncJob* pJob = new (std::nothrow) SyncJob(*(toReschedule->GetSyncJob()));

			if (pJob)
			{
				LOG_LOGD("Reschedule the job");
				g_source_remove(toReschedule->GetTimerId());
				HandleJobCompletedOrCancelledLocked(SYNC_STATUS_CANCELLED, pJob);
				SyncManager::GetInstance()->CloseCurrentSyncContext(toReschedule);
				//SyncManager::GetInstance()->ScheduleSyncJob(pJob);
			}
			else
			{
				LOG_LOGD("Failed to construct SyncJob");
			}
		}
		if (!alreadyInProgress)
		{
			int result = DispatchSyncJob(*pCandidate);
			if (result == SYNC_ERROR_NONE)
			{
				pthread_mutex_lock(&(SyncManager::GetInstance()->__syncJobQueueMutex));
				SyncManager::GetInstance()->__pSyncJobQueue->RemoveSyncJob(pCandidate->key);
				pthread_mutex_unlock(&(SyncManager::GetInstance()->__syncJobQueueMutex));
			}
		}
	}

	for (it = listJobs.begin(); it != listJobs.end(); it++)
	{
		SyncJob* pJob = *it;
		delete pJob;
		pJob = NULL;
	}

	if (nextReadyToRunTime != LLONG_MAX)
	{
		LOG_LOGD("nextReadyToRunTime %lld", nextReadyToRunTime);
	}
	return nextReadyToRunTime;
}



/* Schedules periodic sync jobs which are ready to be turned into pending jobs
 * Returns earliest future start time of periodic sync job, in milliseconds since boot
 */
long long
SyncJobDispatcher::SchedulePeriodicSyncJobs(void)
{
	long long earliestNextRunTime = LLONG_MAX;

	//vector<account_h> accounts = __runningAccounts;

	//TODO: Schedule account less sync jobs

	///map<string, PeriodicSyncJob*>::iterator beginItr = SyncManager::GetInstance()->GetSyncRepositoryEngine()->__accLessPeriodicSyncJobs.begin();
	//vector<CurrentSyncContext*>::iterator it2;
	/*for (beginItr = listCurrentSyncs.begin(); it2 != listCurrentSyncs.end(); it2++)
	{
	SyncJob* pJob = new (std::nothrow) SyncJob(pCapabilityInfo->appId,
												pCapabilityInfo->accountHandle,
												pCapabilityInfo->capability,
												pExtras, REASON_PERIODIC, SOURCE_PERIODIC,
												0, 0, backOffTime,
												(SyncManager::GetInstance())->GetSyncRepositoryEngine()->GetDelayUntilTime(
												pCapabilityInfo->accountHandle, pCapabilityInfo->capability),
												false//TODO replace false with syncAdapterInfo.type.allowParallelSyncs()
												);

	if (!pJob)
	{
		LOG_LOGD("Failed to construct new SyncJob");
		continue;
	}*/

	struct timeval timeVal;
	gettimeofday(&timeVal, NULL);
	long long currentTimeInMilliSecs = timeVal.tv_sec * 1000LL + timeVal.tv_usec / 1000;
	long long shiftedFromCurrentTime = (0 < currentTimeInMilliSecs - SyncManager::GetInstance()->__randomOffsetInMillis) ? (currentTimeInMilliSecs - SyncManager::GetInstance()->__randomOffsetInMillis) : 0;

	vector<pair<CapabilityInfo*, SyncStatusInfo*> > allCapabilitiesAndStatusInfo;
	if ((SyncManager::GetInstance())->GetSyncRepositoryEngine())
	{
		allCapabilitiesAndStatusInfo = (SyncManager::GetInstance())->GetSyncRepositoryEngine()->GetCopyOfAllCapabilityAndSyncStatusN();
	}

	if (allCapabilitiesAndStatusInfo.size() == 0)
	{
		LOG_LOGD("Capability list is zero");
		return LLONG_MAX;
	}
	LOG_LOGD("Capability list size %d", allCapabilitiesAndStatusInfo.size());

	for (unsigned int i = 0; i < allCapabilitiesAndStatusInfo.size(); i++)
	{
		CapabilityInfo* pCapabilityInfo = allCapabilitiesAndStatusInfo[i].first;
		SyncStatusInfo* pSyncStatusInfo = allCapabilitiesAndStatusInfo[i].second;
		if (pCapabilityInfo == NULL || pSyncStatusInfo == NULL)
		{
			continue;
		}

		if (pCapabilityInfo->capability.empty())
		{
			LOG_LOGD("Empty capability, skipping");
			delete pCapabilityInfo;
			delete pSyncStatusInfo;
			pCapabilityInfo = NULL;
			pSyncStatusInfo = NULL;
			continue;
		}
		/*if (!IsActiveAccount(accounts, pCapabilityInfo->accountHandle))
		{
			LOG_LOGD("This account is no longer running, skipping");
			delete pCapabilityInfo;
			delete pSyncStatusInfo;
			continue;
		}*/

		if (!((SyncManager::GetInstance())->GetSyncRepositoryEngine()->GetSyncAutomatically(pCapabilityInfo->accountHandle, pCapabilityInfo->capability))
				//TODO && !GetMasterSyncAutomatically()
				)
		{
			delete pCapabilityInfo;
			delete pSyncStatusInfo;
			pCapabilityInfo = NULL;
			pSyncStatusInfo = NULL;
			continue;
		}

		if ((SyncManager::GetInstance())->GetSyncable(pCapabilityInfo->accountHandle, pCapabilityInfo->capability) == 0)
		{
			delete pCapabilityInfo;
			delete pSyncStatusInfo;
			pCapabilityInfo = NULL;
			pSyncStatusInfo = NULL;
			continue;
		}

		LOG_LOGD("Capability [%s] PeriodicJobs [%d]", pCapabilityInfo->capability.c_str(), pCapabilityInfo->periodicSyncList.size());
		if (pCapabilityInfo->periodicSyncList.size() == 0)
		{
			continue;
		}

		for (unsigned int j = 0; j < pCapabilityInfo->periodicSyncList.size(); j++)
		{
			PeriodicSyncJob* pPeriodicSyncJob = pCapabilityInfo->periodicSyncList[j];

			bundle* pExtras = bundle_dup(pPeriodicSyncJob->pExtras);

			long long periodInMillis = pPeriodicSyncJob->period * 1000LL;
			long long flexiTimeInMillis = pPeriodicSyncJob->flexTime * 1000LL;
			if (periodInMillis <= 0)
			{
				continue;
			}

			long long lastRunTime = pSyncStatusInfo->GetPeriodicSyncTime(j);
			long long remainingMillis = periodInMillis - (shiftedFromCurrentTime % periodInMillis);
			long long timeSinceLastRunInMillis = currentTimeInMilliSecs - lastRunTime;
			bool canStartEarly = (remainingMillis <= flexiTimeInMillis) && (timeSinceLastRunInMillis > periodInMillis - flexiTimeInMillis);

			LOG_LOGD("period [%lld ms] flexi [%lld ms] remaining [%lld ms] time since last run [%lld ms] shift from current time [%lld ms]", periodInMillis, flexiTimeInMillis, remainingMillis, timeSinceLastRunInMillis, shiftedFromCurrentTime);
			LOG_LOGD("Can start early %s", canStartEarly ? "YES" : "NO");

			/* Sync scheduling algorithm : set next periodic sync based on a random offset in seconds.
			 * Sync right now, if any of the below cases is true and mark it as scheduled.
			 *
			 * Case 1: Sync is ready to run now
			 * Case 2: If the lastRunTime is in future, this can happen if user changes time, sync happens and user changes time back
			 * Case 3: If sync failed at last scheduled time
			 */
			if (canStartEarly														//
					|| remainingMillis == periodInMillis							// Case 1
					|| lastRunTime > currentTimeInMilliSecs							// Case 2
					|| timeSinceLastRunInMillis >= periodInMillis )					// Case 3
			{
				LOG_LOGD("Scheduling a Periodic sync job appid [%s], period:[%lld ms], time since last run [%lld ms]", pCapabilityInfo->appId.c_str(), periodInMillis, timeSinceLastRunInMillis);
				backOff* pBackOff = (SyncManager::GetInstance())->GetSyncRepositoryEngine()->GetBackoffN(pCapabilityInfo->accountHandle, pCapabilityInfo->capability);
				long backOffTime = pBackOff ? pBackOff->time : 0;
				delete pBackOff;
				pBackOff = NULL;

				//TODO Check for SyncAdapterInfo for this capability using RegisteredServiceCache serviceInfo
				// If SyncAdapterInfo is null for this capability, continue.

				SyncManager::GetInstance()->GetSyncRepositoryEngine()->SetPeriodicSyncTime(pCapabilityInfo->id, pPeriodicSyncJob, currentTimeInMilliSecs);

				SyncJob* pJob = new (std::nothrow) SyncJob(pCapabilityInfo->appId,
															pCapabilityInfo->accountHandle,
															pCapabilityInfo->capability,
															pExtras, REASON_PERIODIC, SOURCE_PERIODIC,
															0, 0, backOffTime,
															(SyncManager::GetInstance())->GetSyncRepositoryEngine()->GetDelayUntilTime(
															pCapabilityInfo->accountHandle, pCapabilityInfo->capability),
															false//TODO replace false with syncAdapterInfo.type.allowParallelSyncs()
															);

				if (!pJob)
				{
					LOG_LOGD("Failed to construct new SyncJob");
					continue;
				}

				account_sync_state_e syncSupport;
				int ret = account_get_sync_support(pCapabilityInfo->accountHandle,  &syncSupport);
				if (ret < 0)
				{
					LOG_LOGD("account_get_sync_support failed");
				}
				if (syncSupport == (int)ACCOUNT_SYNC_INVALID || syncSupport == (int)ACCOUNT_SYNC_NOT_SUPPORT)
				{
					LOG_LOGD("this account does not support sync");
				}
				else
				{
					SyncManager::GetInstance()->ScheduleSyncJob(pJob, false);
				}
				delete pJob;
				pJob = NULL;
			}
			else
			{
				LOG_LOGD("Skipping periodic sync job appid [%s], period:[%lld ms], timeSinceLastRunInMillis [%lld ms]", pCapabilityInfo->appId.c_str(), periodInMillis, timeSinceLastRunInMillis);
			}

			// Compute when this periodic sync should run next
			long long nextRunTime = currentTimeInMilliSecs + periodInMillis;
			if (nextRunTime < earliestNextRunTime)
			{
				earliestNextRunTime = nextRunTime;
			}
		}
		delete pCapabilityInfo;
		pCapabilityInfo = NULL;
		delete pSyncStatusInfo;
		pSyncStatusInfo = NULL;
	}
	if (earliestNextRunTime == LLONG_MAX)
	{
		LOG_LOGD("Nothing to schedule");
		return LLONG_MAX;
	}

	return (earliestNextRunTime < currentTimeInMilliSecs) ? 0 : (earliestNextRunTime - currentTimeInMilliSecs);
}


//}//_SyncManager
