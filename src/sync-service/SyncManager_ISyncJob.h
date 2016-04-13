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
 * @file	SyncManager_ISyncJob.h
 * @brief	This is the header file for the SyncJob class.
 */

#ifndef SYNC_SERVICE_ISYNC_JOB_H
#define SYNC_SERVICE_ISYNC_JOB_H


/*namespace _SyncManager
{
*/


#ifdef __cplusplus
extern "C" {
#endif

using namespace std;


enum SyncType {
	SYNC_TYPE_ON_DEMAND = 0,
	SYNC_TYPE_PERIODIC,
	SYNC_TYPE_DATA_CHANGE,
	SYNC_TYPE_UNKNOWN
};


class ISyncJob {
public:
	ISyncJob()
		: __syncJobId(-1), __syncType(SYNC_TYPE_UNKNOWN) {}

	ISyncJob(int syncJobId, SyncType type)
			: __syncJobId(syncJobId)
			, __syncType(type) {}

	virtual ~ISyncJob() {}

	virtual SyncType GetSyncType() = 0;

	virtual int GetSyncJobId() {
		return __syncJobId;
	}

protected:
	int __syncJobId;
	SyncType __syncType;
};


#ifdef __cplusplus
}
#endif

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_ISYNC_JOB_H */
