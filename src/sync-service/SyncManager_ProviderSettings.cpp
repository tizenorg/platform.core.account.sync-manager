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
 * @file	SyncManager_ProviderSettings.cpp
 * @brief	This is the implementation file for the ProviderSetting class.
 */

#include "SyncManager_ProviderSettings.h"
#include "sync-log.h"


/*namespace _SyncManager
{*/

ProviderSetting::ProviderSetting(void)
	: __accountTypeId(-1),
	  __isVisible(0),
	  __isUploadSupported(0),
	  __isParallelSyncAllowed(0),
	  __isAlwaysSyncable(1)
{
	LOG_LOGD("ProviderSetting::ProviderSetting");
}


ProviderSetting::~ProviderSetting(void)
{
	LOG_LOGD("ProviderSetting::~ProviderSetting");
}


int
ProviderSetting::GetAccountTypeId(void) const
{
	return __accountTypeId;
}


bool
ProviderSetting::GetUserVisible(void) const
{
	return __isVisible;
}


bool
ProviderSetting::GetSupportsUploading(void) const
{
	return __isUploadSupported;
}


bool
ProviderSetting::GetAllowParallelSync(void) const
{
	return __isParallelSyncAllowed;
}


bool
ProviderSetting::GetAlwaysSyncable(void) const
{
	return __isAlwaysSyncable;
}


void
ProviderSetting::SetAccountTypeId(int accountTypeId)
{
	__accountTypeId = accountTypeId;
}


void
ProviderSetting::SetUserVisible(bool isVisible)
{
	__isVisible = isVisible;
}


void
ProviderSetting::SetSupportsUploading(bool isUploadSupported)
{
	__isUploadSupported = isUploadSupported;
}


void
ProviderSetting::SetAllowParallelSync(bool isParallelSyncAllowed)
{
	__isParallelSyncAllowed = isParallelSyncAllowed;
}


void
ProviderSetting::SetAlwaysSyncable(bool isAlwaysSyncable)
{
	__isAlwaysSyncable = isAlwaysSyncable;
}
//}//_SyncManager
