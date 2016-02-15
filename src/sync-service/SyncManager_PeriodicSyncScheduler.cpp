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
 * @file    SyncManager_PeriodicSyncScheduler.cpp
 * @brief   This is the implementation file for the PeriodicSyncScheduler class.
 */


#if defined(_SEC_FEATURE_CALENDAR_CONTACTS_ENABLE)
#include <calendar.h>
#include <contacts.h>
#endif

#include <media_content.h>
#include "sync-error.h"
#include "SyncManager_PeriodicSyncScheduler.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncManager.h"


PeriodicSyncScheduler::~PeriodicSyncScheduler(void)
{
}


PeriodicSyncScheduler::PeriodicSyncScheduler(void)
{
}


int
PeriodicSyncScheduler::OnAlarmExpired(alarm_id_t alarm_id, void *user_param)
{
	LOG_LOGD("Alarm id %d", alarm_id);

	PeriodicSyncScheduler* pPeriodicSyncScheduler = (PeriodicSyncScheduler*) user_param;
	map<int, PeriodicSyncJob*>::iterator itr = pPeriodicSyncScheduler->__activePeriodicSyncJobs.find(alarm_id);

	if(itr != pPeriodicSyncScheduler->__activePeriodicSyncJobs.end()) {
		PeriodicSyncJob* pSyncJob = pPeriodicSyncScheduler->__activePeriodicSyncJobs[alarm_id];
		LOG_LOGD("Alarm expired for [%s]", pSyncJob->__key.c_str());
		SyncManager::GetInstance()->ScheduleSyncJob(pSyncJob, true);
	}

	/*SyncJob* pJob = new (std::nothrow) SyncJob(pSyncJob->__appId,
												pSyncJob->__accountId,
												pSyncJob->__syncJobName,
												pSyncJob->__pExtras,
												pSyncJob->__isExpedited
												);*/

	return true;
}


int
PeriodicSyncScheduler::RemoveAlarmForPeriodicSyncJob(PeriodicSyncJob* pSyncJob)
{
	LOG_LOGD("Removing alarm for periodic sync job");

	string jobKey = pSyncJob->__key;
	map<string, int>::iterator iter = __activeAlarmList.find(jobKey);

	if (iter != __activeAlarmList.end()) {
		alarm_id_t alarm = iter->second;
		int ret = alarmmgr_remove_alarm(alarm);
		SYNC_LOGE_RET_RES(ret == ALARMMGR_RESULT_SUCCESS, SYNC_ERROR_SYSTEM, "alarm remove failed for [%s], [%d]", jobKey.c_str(), ret);

		__activeAlarmList.erase(iter);
		__activePeriodicSyncJobs.erase(alarm);
		LOG_LOGD("Removed alarm for [%s], [%d]", jobKey.c_str(), alarm);
	}
	else {
		LOG_LOGD("No active alarm found for [%s]", jobKey.c_str());
	}

	return SYNC_ERROR_NONE;
}


int
PeriodicSyncScheduler::SchedulePeriodicSyncJob(PeriodicSyncJob* periodicSyncJob)
{
	string jobKey = periodicSyncJob->__key;

	//Remove previous alarms, if set already
	int ret = SYNC_ERROR_NONE;
	ret = RemoveAlarmForPeriodicSyncJob(periodicSyncJob);
	SYNC_LOGE_RET_RES(ret == SYNC_ERROR_NONE, SYNC_ERROR_SYSTEM, "Failed to remove previous alarm for [%s], [%d]", jobKey.c_str(), ret);

	alarm_id_t alarm_id;
	ret = alarmmgr_add_periodic_alarm_withcb(periodicSyncJob->__period, QUANTUMIZE, PeriodicSyncScheduler::OnAlarmExpired, this, &alarm_id);
	//ret = alarmmgr_add_periodic_alarm_withcb(2, QUANTUMIZE, PeriodicSyncScheduler::OnAlarmExpired, this, &alarm_id);
	if (ret == ALARMMGR_RESULT_SUCCESS) {
		//LOG_LOGD("Alarm added for %ld min, id %ld", periodicSyncJob->__period, alarm_id);
		LOG_LOGD("Alarm added for %ld min, id %ld", periodicSyncJob->__period, alarm_id);

		__activePeriodicSyncJobs.insert(make_pair<int, PeriodicSyncJob*> (alarm_id, periodicSyncJob));
		__activeAlarmList.insert(make_pair(jobKey, alarm_id));
	}
	else {
		LOG_LOGD("Failed to add Alarm for %ld min, ret %d", periodicSyncJob->__period, ret);
		return SYNC_ERROR_SYSTEM;
	}

	LOG_LOGD("Active periodic alarm count, %d", __activePeriodicSyncJobs.size());

	return SYNC_ERROR_NONE;
}

