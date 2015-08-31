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
 * @file	SyncManager_SettingsDbHelper.cpp
 * @brief	This is the implementation file for the SettingDBHelper class.
 */


#include "sync-error.h"
#include "sync-log.h"
#include "SyncManager_SettingsDbHelper.h"

/*namespace _SyncManager
{*/

#define SYNC_SERVICE_SETTING_DB_PATH "/opt/usr/dbspace/.sync_service_settings.db"
#define SYNC_SERVICE_SETTING_DB_JOURNAL_PATH "/opt/usr/dbspace/.sync_service_settings.db-journal"
#define SYNC_SETTINGS_TABLE "sync_settings_table"

static const char* SCHEMA_SYNC_SETTINGS_CREATE_QUERY = "CREATE TABLE SYNC_SETTINGS_TABLE(" \
										"_id INTEGER PRIMARY KEY AUTOINCREMENT, "\
										"account_type_id INTEGER NOT NULL UNIQUE, "\
										"UserVisible INTEGER DEFAULT 0 CHECK(UserVisible >= 0 AND UserVisible <= 1), "\
										"SupportsUploading INTEGER DEFAULT 0 CHECK(SupportsUploading >= 0 AND SupportsUploading <= 1), "\
										"AllowParallelSyncs INTEGER DEFAULT 0 CHECK(AllowParallelSyncs >=0 AND AllowParallelSyncs <= 1), "\
										"AlwaysSyncable INTEGER DEFAULT 1 CHECK(AlwaysSyncable >= 0 AND AlwaysSyncable <= 1), "\
										"Key TEXT, "\
										"Value TEXT "\
										");";

int
SettingDBHelper::InsertSyncSettings(const ProviderSetting& providerSettings)
{
	LOG_LOGD("ProviderSetting::insertSynSettings starts");

	if (__pDBHandle == NULL)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return SYNC_ERROR_SYSTEM;
		}
	}

	int ret = 0;
	char query[2048]={0,};

	//pthread_mutex_lock(&__mutex);

	snprintf(query, sizeof(query),
			"INSERT INTO %s (account_type_id, UserVisible, SupportsUploading, AllowParallelSyncs, AlwaysSyncable) VALUES(%d, %d, %d, %d, %d)",
			SYNC_SETTINGS_TABLE,
			providerSettings.GetAccountTypeId(),
			providerSettings.GetUserVisible(),
			providerSettings.GetSupportsUploading(),
			providerSettings.GetAllowParallelSync(),
			providerSettings.GetAlwaysSyncable());

	ret = ExecDb(query);

	//pthread_mutex_unlock(&__mutex);

	if (ret != 0)
	{
		LOG_ERRORD("ExecDb() failed for insert");
		return SYNC_ERROR_SYSTEM;
	}

	LOG_LOGD("ProviderSetting::insertSynSettings ends");

	return SYNC_ERROR_NONE;
}


int
SettingDBHelper::UpdateSyncSettings(const ProviderSetting& providerSettings)
{
	LOG_LOGD("ProviderSetting::updateSyncSettings starts");

	if (__pDBHandle == NULL)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return SYNC_ERROR_SYSTEM;
		}
	}

	int ret = 0;
	char query[2048]={0,};

	//pthread_mutex_lock(&__mutex);

	snprintf(query, sizeof(query),
			"UPDATE  %s SET UserVisible = %d, SupportsUploading = %d, AllowParallelSyncs = %d, AlwaysSyncable = %d WHERE account_type_id = %d",
			SYNC_SETTINGS_TABLE,
			providerSettings.GetUserVisible(),
			providerSettings.GetSupportsUploading(),
			providerSettings.GetAllowParallelSync(),
			providerSettings.GetAlwaysSyncable(),
			providerSettings.GetAccountTypeId());

	ret = ExecDb(query);

	//pthread_mutex_unlock(&__mutex);

	if (ret != 0)
	{
		LOG_ERRORD("ExecDb() failed for update");
		return SYNC_ERROR_SYSTEM;
	}


	LOG_LOGD("ProviderSetting::updateSyncSettings ends");
	return SYNC_ERROR_NONE;
}


int
SettingDBHelper::DeleteSyncSettings(const int accountTypeId)
{
	LOG_LOGD("ProviderSetting::deleteSyncSettings starts");

	if (__pDBHandle == NULL)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return SYNC_ERROR_SYSTEM;
		}
	}

	int ret = 0;
	char query[2048]={0,};

	//pthread_mutex_lock(&__mutex);

	snprintf(query, sizeof(query),
			"DELETE FROM  %s WHERE account_type_id = %d", SYNC_SETTINGS_TABLE, accountTypeId);

	ret = ExecDb(query);

	//pthread_mutex_unlock(&__mutex);

	if (ret != 0)
	{
		LOG_ERRORD("ExecDb() failed for delete");
		return SYNC_ERROR_SYSTEM;
	}

	LOG_LOGD("ProviderSetting::deleteSyncSettings ends");
	return SYNC_ERROR_NONE;
}


bool
SettingDBHelper::HasProviderSettings(const int accountTypeId)
{
	LOG_LOGD("ProviderSetting::hasProviderSettings starts");

	if (NULL == __pDBHandle)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return false;
		}
	}

	int ret = 0;
	char query[2048]={0,};
	sqlite3_stmt *stmt = NULL;

	//pthread_mutex_lock(&__mutex);

	snprintf(query, sizeof(query), "select _id FROM %s WHERE account_type_id=%d", SYNC_SETTINGS_TABLE, accountTypeId);

	ret = sqlite3_prepare_v2(__pDBHandle, query, strlen(query), &stmt, NULL);
	if (SQLITE_OK != ret)
	{
		LOG_ERRORD("sqlite3_prepare_v2(%s) failed(%s).", query, sqlite3_errmsg(__pDBHandle));
		//pthread_mutex_unlock(&__mutex);
		return false;
	}

	ret = sqlite3_step(stmt);
	if (SQLITE_ROW != ret)
	{
		sqlite3_finalize(stmt);
		if (SQLITE_DONE != ret)
		{
			LOG_ERRORD("sqlite3_step() failed(%d, %s).", ret, sqlite3_errmsg(__pDBHandle));
		}
		//pthread_mutex_unlock(&__mutex);
		return false;
	}
	sqlite3_finalize(stmt);

	//pthread_mutex_unlock(&__mutex);

	LOG_LOGD("ProviderSetting::hasProviderSettings end");

	return true;
}


int
SettingDBHelper::GetSyncSettings(const int accountTypeId, ProviderSetting& providerSettings)
{
	LOG_LOGD("ProviderSetting::getSyncSettings starts");

	if (__pDBHandle == NULL)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return SYNC_ERROR_SYSTEM;
		}
	}

	int ret = 0;
	char query[2048]={0,};
	sqlite3_stmt* pStmt = NULL;

	//pthread_mutex_lock(&__mutex);

	snprintf(query, sizeof(query), "select * FROM %s WHERE account_type_id=%d", SYNC_SETTINGS_TABLE, accountTypeId);

	ret = sqlite3_prepare_v2(__pDBHandle, query, strlen(query), &pStmt, NULL);
	if (SQLITE_OK != ret)
	{
		LOG_ERRORD("sqlite3_prepare_v2(%s) failed(%s).", query, sqlite3_errmsg(__pDBHandle));
		//pthread_mutex_unlock(&__mutex);
		return SYNC_ERROR_SYSTEM;
	}

	ret = sqlite3_step(pStmt);
	if (SQLITE_ROW != ret)
	{
		sqlite3_finalize(pStmt);
		if (SQLITE_DONE != ret)
		{
			LOG_ERRORD("sqlite3_step() failed(%d, %s).", ret, sqlite3_errmsg(__pDBHandle));
		}
		//pthread_mutex_unlock(&__mutex);
		return SYNC_ERROR_SYSTEM;
	}

	providerSettings.SetAccountTypeId(sqlite3_column_int(pStmt, 1));
	providerSettings.SetUserVisible(sqlite3_column_int(pStmt, 2));
	providerSettings.SetSupportsUploading(sqlite3_column_int(pStmt,3));
	providerSettings.SetAllowParallelSync(sqlite3_column_int(pStmt, 4));
	providerSettings.SetAlwaysSyncable(sqlite3_column_int(pStmt, 5));

	sqlite3_finalize(pStmt);

	//pthread_mutex_unlock(&__mutex);

	LOG_LOGD("ProviderSetting::getSyncSettings starts");

	return SYNC_ERROR_NONE;
}

//int
//SettingDBHelper::getAllSyncSettings(std::list<ProviderSettings>& providerSettings)
//{
//	return SYNC_ERROR_NONE;
//}

SettingDBHelper::SettingDBHelper(void)
{
	__pDBHandle = NULL;
	int ret = pthread_mutex_init(&__mutex, NULL);
	if (ret != 0)
	{
		LOG_ERRORD("Mutex Initialise Failed %d", ret);
		return;
	}
}

SettingDBHelper::~SettingDBHelper(void)
{
	pthread_mutex_destroy(&__mutex);

	if (__pDBHandle)
	{
		db_util_close(__pDBHandle);
		__pDBHandle = NULL;
	}
}


int
SettingDBHelper::CheckDb(void)
{
	int fd = open(SYNC_SERVICE_SETTING_DB_PATH, O_RDONLY);

	if (fd == -1)
	{
		return SYNC_ERROR_IO_ERROR;
	}

	close(fd);

	return SYNC_ERROR_NONE;
}


int
SettingDBHelper::MakeDb(void)
{
	int ret;
	sqlite3* pDb;
	char* pErrMsg;

	ret = db_util_open(SYNC_SERVICE_SETTING_DB_PATH, &pDb, 0);
	if (ret != SQLITE_OK)
	{
		LOG_ERRORD("db_util_open() Failed:%d", ret);
		return SYNC_ERROR_IO_ERROR;
	}
	else
	{
		ret = sqlite3_exec(pDb, SCHEMA_SYNC_SETTINGS_CREATE_QUERY, NULL, NULL, &pErrMsg);
		if (ret != SQLITE_OK)
		{
			LOG_ERRORD("sqlite3_exec() Failed:%d", ret);
			sqlite3_free(pErrMsg);
			return SYNC_ERROR_IO_ERROR;
		}
	}

	db_util_close(pDb);

	return SYNC_ERROR_NONE;
}


int
SettingDBHelper::OpenDb(void)
{
	int ret = 0;

	if (CheckDb() != SYNC_ERROR_NONE)
	{
		ret = MakeDb();
	}

	if ( ret != SYNC_ERROR_NONE)
	{
		LOG_ERRORD("makeDb() failed");
		return SYNC_ERROR_IO_ERROR;
	}

	if (__pDBHandle == NULL)
	{
		ret = db_util_open(SYNC_SERVICE_SETTING_DB_PATH, &__pDBHandle, 0);
		if (ret != SQLITE_OK)
		{
			LOG_ERRORD("db_util_open fail");
			return SYNC_ERROR_IO_ERROR;
		}
	}

	return SYNC_ERROR_NONE;
}


int
SettingDBHelper::ExecDb(const std::string& query)
{
	int ret;
	sqlite3_stmt* pStmt = NULL;

	if (__pDBHandle == NULL)
	{
		if (OpenDb() != SYNC_ERROR_NONE)
		{
			LOG_ERRORD("Database is not opened");
			return SYNC_ERROR_IO_ERROR;
		}
	}
	ret = sqlite3_prepare_v2(__pDBHandle, query.c_str(), strlen(query.c_str()), &pStmt, NULL );
	if (SQLITE_OK != ret)
	{
		LOG_ERRORD("sqlite3_prepare_v2(%s) failed(%d).", query.c_str(), ret);
		return -1;
	}
	ret = sqlite3_step(pStmt);

	if (ret != SQLITE_ROW && ret != SQLITE_DONE)
	{
		LOG_ERRORD("sqlite3_step(%s) failed(%d).", query.c_str(), ret);
		sqlite3_finalize(pStmt);
		if (ret == SQLITE_FULL)
		{
			LOG_ERRORD("full");
			return SYNC_ERROR_OUT_OF_MEMORY;
		}
		return SYNC_ERROR_INVALID_OPERATION;
	}
	sqlite3_finalize(pStmt);

	return SYNC_ERROR_NONE;
}
//}//_SyncManager
