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
 * @file	SyncManager_StorageChangeListener.cpp
 * @brief	This is the implementation file for the StorageChangeListener class.
 */

#include <vconf.h>
#include <vconf-keys.h>
#include <vconf-internal-dnet-keys.h>
#include "SyncManager_SyncManager.h"
#include "SyncManager_StorageChangeListener.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{*/

void OnMemoryStatusChanged(keynode_t* pKey, void* pData)
{
	MemoryStatus value =  static_cast<MemoryStatus> (vconf_keynode_get_int(pKey));

	SyncManager::GetInstance()->OnStorageStatusChanged(value);
}

StorageChangeListener::StorageChangeListener(void)
{
}

StorageChangeListener::~StorageChangeListener(void)
{
}

int
StorageChangeListener::RegisterStorageChangeListener(void)
{
	return( vconf_notify_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, OnMemoryStatusChanged, NULL) );
}

int
StorageChangeListener::DeRegisterStorageChangeListener(void)
{
	LOG_LOGD("Remove storage listener");

	return(vconf_ignore_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, OnMemoryStatusChanged));
}
//}//_SyncManager
