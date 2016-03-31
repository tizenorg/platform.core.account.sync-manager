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
 * @file    SyncManager_PeriodicSyncScheduler.h
 * @brief   This is the header file for the PeriodicSyncScheduler class.
 */

#ifndef _SYNC_SERVICE_PERIODIC_SYNC_SCHEDULER_H_
#define _SYNC_SERVICE_PERIODIC_SYNC_SCHEDULER_H_

#include <alarm.h>
#include <map>
#include "SyncManager_PeriodicSyncJob.h"

class PeriodicSyncScheduler
{
public:
	PeriodicSyncScheduler(void);

	~PeriodicSyncScheduler(void);

	static int OnAlarmExpired(alarm_id_t alarm_id, void *user_param);

	int RemoveAlarmForPeriodicSyncJob(PeriodicSyncJob* pSyncJob);

	int SchedulePeriodicSyncJob(PeriodicSyncJob* dataSyncJob);

private:
	std::map < int, PeriodicSyncJob * > __activePeriodicSyncJobs;

	std::map < string, int > __activeAlarmList;
};

#endif /* _SYNC_SERVICE_DATA_CHANGE_SYNC_SCHEDULER_H_ */
