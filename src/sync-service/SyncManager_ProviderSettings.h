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
 * @file	SyncManager_ProviderSettings.h
 * @brief	This is the header file for the ProviderSetting class.
 */

#ifndef SYNC_SERVICE_PROVIDER_SETTINGS_H
#define SYNC_SERVICE_PROVIDER_SETTINGS_H


/*namespace _SyncManager
{
*/
class ProviderSetting
{
public:
	ProviderSetting(void);

	virtual ~ProviderSetting(void);

	void SetAccountTypeId(int accountTypeId);

	int GetAccountTypeId(void) const;

	void SetUserVisible(bool isVisible);

	bool GetUserVisible(void) const;

	void SetSupportsUploading(bool isUploadSupported);

	bool GetSupportsUploading(void) const;

	void SetAllowParallelSync(bool isParallelSyncAllowed);

	bool GetAllowParallelSync(void) const;

	void SetAlwaysSyncable(bool isAlwaysSyncable);

	bool GetAlwaysSyncable(void) const;

private:

	int __accountTypeId;

	bool __isVisible;

	bool __isUploadSupported;

	bool __isParallelSyncAllowed;

	bool __isAlwaysSyncable;
};
//}//_SyncManager
#endif // SYNC_SERVICE_PROVIDER_SETTINGS_H
