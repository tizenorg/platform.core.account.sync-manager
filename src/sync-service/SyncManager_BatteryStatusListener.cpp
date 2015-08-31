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
 * @file	SyncManager_BatteryStatusListener.cpp
 * @brief	This is the implementation file for the BatteryStatusListener class.
 */

#include <vconf.h>
#include <vconf-keys.h>
#include <vconf-internal-dnet-keys.h>
#include "SyncManager_SyncManager.h"
#include "SyncManager_BatteryStatusListener.h"
#include "SyncManager_SyncDefines.h"
#include "sync-log.h"


/*namespace _SyncManager
{*/


void OnBatteryStatusChanged(keynode_t* pKey, void* pData)
{
	LOG_LOGD("OnBatteryStatusChanged Starts");

	BatteryStatus value =  static_cast<BatteryStatus> (vconf_keynode_get_int(pKey));

	SyncManager::GetInstance()->OnBatteryStatusChanged(value);

	LOG_LOGD("OnBatteryStatusChanged Ends");
}


BatteryStatusListener::BatteryStatusListener(void)
{
}


BatteryStatusListener::~BatteryStatusListener(void)
{
}


int
BatteryStatusListener::RegisterBatteryStatusListener(void)
{
	return(vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, OnBatteryStatusChanged, NULL));
}


int
BatteryStatusListener::DeRegisterBatteryStatusListener(void)
{
	LOG_LOGD("DeRegisterBatteryStatusListener");

	return(vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, OnBatteryStatusChanged));
}
//}//_SyncManager
