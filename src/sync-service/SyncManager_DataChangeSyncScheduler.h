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
 * @file    SyncManager_DataChangeSyncScheduler.h
 * @brief   This is the header file for the DataChangeSyncScheduler class.
 */

#ifndef _SYNC_SERVICE_DATA_CHANGE_SYNC_SCHEDULER_H_
#define _SYNC_SERVICE_DATA_CHANGE_SYNC_SCHEDULER_H_

#include "SyncManager_DataSyncJob.h"
#include <map>

class DataChangeSyncScheduler {
public:
	bool calendar_subscription_started;
	bool contacts_subscription_started;
	bool media_content_subscription_started;

public:
	DataChangeSyncScheduler(void);

	~DataChangeSyncScheduler(void);

	int RegisterDataChangeListeners(void);

	int DeRegisterDataChangeListeners(void);

	int AddDataSyncJob(string capability, DataSyncJob* dataSyncJob);

	void RemoveDataSyncJob(DataSyncJob* dataSyncJob);

	void HandleDataChangeEvent(const char* syncCapability);

	/* void OnCalendarDataChanged(int value); */

	/* void OnContactsDataChanged(int value); */

	/* void OnMediaContentDataChanged(media_content_type_e media_content_type); */

private:
	int SubscribeCalendarCallback(void);

	int SubscribeContactsCallback(void);

	int SubscribeMediaContentCallback(void);

	int UnSubscribeCalendarCallback(void);

	int UnSubscribeContactsCallback(void);

	int UnSubscribeMediaContentCallback(void);

	std::multimap < string, DataSyncJob * > __dataChangeSyncJobs;
};

#endif /* _SYNC_SERVICE_DATA_CHANGE_SYNC_SCHEDULER_H_ */
