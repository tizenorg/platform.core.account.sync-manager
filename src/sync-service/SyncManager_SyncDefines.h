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

#ifndef SYNC_SERVICE_SYNC_DEFINES_H
#define SYNC_SERVICE_SYNC_DEFINES_H

#include <account.h>
#include <string>
#if defined(_FEATURE_CONTAINER_ENABLE)
#include <vasum.h>
#endif

#define VALUE_UNDEFINED -1

/*namespace _SyncManager
{
*/

#if defined(_FEATURE_CONTAINER_ENABLE)
#define KNOX_CONTAINER_ZONE_ENTER(pid)	struct vsm_context *ctx; \
									struct vsm_zone* effective_zone; \
									ctx = vsm_create_context();\
									effective_zone = (pid == -1) ? vsm_get_foreground(ctx) : vsm_lookup_zone_by_pid(ctx, pid);\
									vsm_zone* prev_zone = vsm_join_zone(effective_zone);

#define KNOX_CONTAINER_ZONE_EXIT()	vsm_zone* zone = vsm_join_zone(prev_zone);
#else
#define KNOX_CONTAINER_ZONE_ENTER(pid)
#define KNOX_CONTAINER_ZONE_EXIT()

#endif


typedef int account_id;

enum BluetoothStatus {
	/** Bluetooth OFF */
	BT_OFF,
	/** Bluetooth ON */
	BT_ON,
	/** Discoverable mode */
	BT_VISIBLE,
	/** In transfering */
	BT_TRANSFER
};


enum WifiStatus {
	/** power off */
	WIFI_OFF,
	/** power on */
	WIFI_NOT_CONNECTED,
	/** connected */
	WIFI_CONNECTED
};


enum DNetStatus {
	/** not connected */
	DNET_OFF = 0x00,
	/** connected */
	DNET_NORMAL_CONNECTED,
	/** secure connected */
	DNET_SECURE_CONNECTED,
	/** patcket transmitted */
	DNET_TRANSFER,
	DNET_STATE_MAX
};


enum WifiDirect {
	/** Power off */
	WIFI_DIRECT_DEACTIVATED  = 0,
	/** Power on */
	WIFI_DIRECT_ACTIVATED,
	/** Discoverable mode */
	WIFI_DIRECT_DISCOVERING,
	/** Connected with peer as GC */
	WIFI_DIRECT_CONNECTED,
	/** Connected with peer as GO */
	WIFI_DIRECT_GROUP_OWNER
};


enum MemoryStatus {
	/** Normal */
	LOW_MEMORY_NORMAL = 0x01,
	/** 60M and under */
	LOW_MEMORY_SOFT_WARNING = 0x02,
	/** 40M and under */
	LOW_MEMORY_HARD_WARNING = 0x04
};


enum BatteryStatus {
	/** 1% and under */
	BAT_POWER_OFF = 1,
	/** 5% and under */
	BAT_CRITICAL_LOW,
	/** 15% and under */
	BAT_WARNING_LOW,
	/** over 15% */
	BAT_NORMAL,
	/** full */
	BAT_FULL,
	/** power off */
	BAT_REAL_POWER_OFF
};


enum SyncDispatchMessage {
	/** Sync Finished*/
	SYNC_FINISHED = 0,
	/** Sync Alaram*/
	SYNC_ALARM,
	/** Check Alaram */
	SYNC_CHECK_ALARM,
	/** Sync Cancel */
	SYNC_CANCEL
};


enum SyncReason {
	/** User initiated */
	REASON_USER_INITIATED = -1,
	/** Settings Changed */
	REASON_DATA_SETTINGS_CHANGED = -2,
	/** Periodic */
	REASON_PERIODIC = -3,
	/** Service Changed */
	REASON_SERVICE_CHANGED = -4,
	/** Account Updated */
	REASON_ACCOUNT_UPDATED = -5,
	/** Auto Sync */
	REASON_AUTO_SYNC = -6,
	/** Change in calendar/contacts/image/music/sound/video data */
	REASON_DEVICE_DATA_CHANGED = -7
};


enum SyncSource {
	/** User initated*/
	SOURCE_USER = 0,
	/** Server initiated */
	SOURCE_SERVER,
	/** Periodic sync */
	SOURCE_PERIODIC,
	/** Poll based, like on connection to network */
	SOURCE_POLL,
	/** local-initiated source */
	SOURCE_LOCAL
};


class BackOffMode {
public:
	static long NOT_IN_BACKOFF_MODE;
};

/*
typedef struct SyncStatus
{
	static bool syncCancel;
	static bool syncSuccess;
	static bool syncFailure;
	static bool syncError;
	static bool syncAlreadyInProgress;
	static bool syncTooManyDelets;
	static bool fullSyncRequested;
	static long delayUntill;
	static bool syncAlarm;
	static bool syncTooManyTries;
	static bool syncSomeProgress;
}SyncStatus;
*/


enum SyncStatus {
	SYNC_STATUS_SUCCESS = 0,
	SYNC_STATUS_CANCELLED =  -1,
	SYNC_STATUS_SYNC_ALREADY_IN_PROGRESS = -2,
	SYNC_STATUS_FAILURE = -3,
	SYNC_STATUS_UNKNOWN = -4
};


#define SYNC_JOB_LIMIT 100
class SyncJob;


struct Message {
	Message() {
		acc = NULL;
		pSyncJob = NULL;
		res = SYNC_STATUS_UNKNOWN;
		type = SYNC_CHECK_ALARM;
	}

	std::string capability;

	SyncDispatchMessage type;
	account_h acc;
	SyncStatus res;
	SyncJob* pSyncJob;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_DEFINES_H */
