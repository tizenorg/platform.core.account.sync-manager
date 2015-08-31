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
 * @file	SyncManager_SettingsDbHelper.h
 * @brief	This is the header file for the SettingDBHelper class.
 */

#ifndef SYNC_SERVICE_SETTINGS_DB_HELPER_H
#define SYNC_SERVICE_SETTINGS_DB_HELPER_H

#include <stdlib.h>
#include <string>
#include <db-util.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include "SyncManager_ProviderSettings.h"
#include "SyncManager_Singleton.h"

/*namespace _SyncManager
{
*/
using namespace std;

class SettingDBHelper
	: public Singleton<SettingDBHelper>
{
public:
	int InsertSyncSettings(const ProviderSetting& providerSettings);

	int UpdateSyncSettings(const ProviderSetting& providerSettings);

	int DeleteSyncSettings(const int accountTypeId);

	bool HasProviderSettings(const int accountTypeId);

	int GetSyncSettings(const int accountTypeId, ProviderSetting& providerSettings);
	//int GetAllSyncSettings(std::list<ProviderSettings>& providerSettings);

protected:
	SettingDBHelper(void);

	~SettingDBHelper(void);

	friend class Singleton<SettingDBHelper>;

private:
	SettingDBHelper(const SettingDBHelper&);

	const SettingDBHelper& operator=(const SettingDBHelper&);

	int CheckDb(void);

	int MakeDb(void);

	int OpenDb(void);

	int ExecDb(const std::string& query);

private:
	sqlite3* __pDBHandle;
	pthread_mutex_t __mutex;
};
//}//_SyncManager
#endif // SYNC_SERVICE_SETTINGS_DB_HELPER_H
