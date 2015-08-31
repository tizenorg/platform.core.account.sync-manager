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


#include <calendar.h>
#include <contacts.h>
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
	PeriodicSyncJob* pSyncJob = pPeriodicSyncScheduler->__activePeriodicSyncJobs[alarm_id];

	LOG_LOGD("Alarm expired for %s %s", pSyncJob->__appId.c_str(), pSyncJob->__syncJobName.c_str());

	/*SyncJob* pJob = new (std::nothrow) SyncJob(pSyncJob->__appId,
												pSyncJob->__accountId,
												pSyncJob->__syncJobName,
												pSyncJob->__pExtras,
												pSyncJob->__isExpedited
												);*/

	SyncManager::GetInstance()->ScheduleSyncJob(pSyncJob, true);

	return true;
}


int
PeriodicSyncScheduler::RemoveAlarmForPeriodicSyncJob(PeriodicSyncJob* pSyncJob)
{
	LOG_LOGD("Removing alarm for periodic sync job");

	string jobKey = pSyncJob->__key;
	map<string, int>::iterator iter = __activeAlarmList.find(jobKey);
	if (iter != __activeAlarmList.end())
	{
		alarm_id_t prevAlarm = iter->second;
		int ret = alarmmgr_remove_alarm(prevAlarm);
		SYNC_LOGE_RET_RES(ret == ALARMMGR_RESULT_SUCCESS, SYNC_ERROR_SYSTEM, "alarm remove failed, %d", ret);
	}
}


int
PeriodicSyncScheduler::SchedulePeriodicSyncJob(PeriodicSyncJob* periodicSyncJob)
{
	string jobKey = periodicSyncJob->__key;
	map<string, int>::iterator iter = __activeAlarmList.find(jobKey);
	if (iter != __activeAlarmList.end())
	{
		alarm_id_t prevAlarm = iter->second;
		int ret = alarmmgr_remove_alarm(prevAlarm);
		SYNC_LOGE_RET_RES(ret != ALARMMGR_RESULT_SUCCESS, SYNC_ERROR_SYSTEM, "alarm remove failed, %d", ret);
	}

	alarm_id_t alarm_id;
	int ret = alarmmgr_add_periodic_alarm_withcb(periodicSyncJob->__period, QUANTUMIZE, PeriodicSyncScheduler::OnAlarmExpired, this, &alarm_id);
	if (ret == ALARMMGR_RESULT_SUCCESS)
	{
		LOG_LOGD("Alarm added for %ld min, id %ld", periodicSyncJob->__period, alarm_id);

		__activePeriodicSyncJobs.insert(make_pair<int, PeriodicSyncJob*> (alarm_id, periodicSyncJob));
		__activeAlarmList.insert(make_pair(jobKey, alarm_id));
	}
	else
	{
		LOG_LOGD("Failed to add Alarm added for %ld min, ret %dd", periodicSyncJob->__period, ret);
	}

	LOG_LOGD("Active periodic alarm count, %d", __activePeriodicSyncJobs.size());
}

