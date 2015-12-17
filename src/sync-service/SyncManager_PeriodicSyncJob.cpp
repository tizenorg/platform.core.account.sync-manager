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
 * @file	SyncManager_PeriodicSyncJob.cpp
 * @brief	This is the implementation file for the PeriodicSyncJob class.
 */

#include "SyncManager_SyncManager.h"
#include "SyncManager_PeriodicSyncJob.h"

/*namespace _SyncManager
{*/


PeriodicSyncJob::~PeriodicSyncJob(void)
{
}


PeriodicSyncJob::PeriodicSyncJob(const string appId, const string syncJobName, int accountId, bundle* pUserData, int syncOption, int syncJobId, SyncType type, long frequency)
	: SyncJob(appId, syncJobName, accountId, pUserData, syncOption, syncJobId, type)
	, __period(frequency)
{
}


PeriodicSyncJob::PeriodicSyncJob(const PeriodicSyncJob& other)
			: SyncJob(other)
			, __period(other.__period)
{
/*	this->__accountHandle = other.__accountHandle;
	this->__capability = other.__capability;
	this->__pExtras = bundle_dup(other.__pExtras);
	this->__period = other.__period;
	this->__syncAdapter = other.__syncAdapter;*/
}


PeriodicSyncJob&
PeriodicSyncJob::operator = (const PeriodicSyncJob& other)
{
	/*this->__accountHandle = other.__accountHandle;
	this->__capability = other.__capability;
	this->__pExtras = bundle_dup(other.__pExtras);
	this->__period = other.__period;
	this->__syncAdapter = other.__syncAdapter;*/

	return *this;
}


bool
PeriodicSyncJob::operator==(const PeriodicSyncJob& other)
{/*
	if ((SyncManager::GetInstance())->AreAccountsEqual(this->__accountHandle, other.__accountHandle)
			&& (this->__capability).compare(other.__capability) == 0
			&& this->__period == other.__period
			&& this->__syncAdapter == other.__syncAdapter
			&& IsExtraEqual((PeriodicSyncJob*)&other)) {
		return true;
	}*/
	return false;
}


bool
PeriodicSyncJob::IsExtraEqual(PeriodicSyncJob* pJob)
{
	/*bundle* pExtra1 = this->__pExtras;
	bundle* pExtra2 = pJob->__pExtras;

	if (pExtra2 == NULL || pExtra1 == NULL)
	{
		return NULL;
	}

	char **argv1;
	int argc1 = bundle_export_to_argv(pExtra1, &argv1);

	char **argv2;
	int argc2 = bundle_export_to_argv(pExtra2, &argv2);

	if (argc1 != argc2)
	{
		return false;
	}

	int index1 = 0;
	int index2 = 0;
	for (index1 = 2; index1 < argc1 ; index1 = index1 + 2)
	{
		const char* pValue1 = bundle_get_val(pExtra1, argv1[index1]);
		string key1(argv1[index1]);
		bool found = false;

		for (index2 = 2; index2 < argc2 ; index2 = index2 + 2)
		{
			const char* pValue2 = bundle_get_val(pExtra2, argv2[index2]);
			string key2(argv2[index2]);

			if (strcmp(pValue1, pValue2) == 0 && key1.compare(key2) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			bundle_free_exported_argv(argc1, &argv1);
			bundle_free_exported_argv(argc2, &argv2);

			return false;
		}
	}

	bundle_free_exported_argv(argc1, &argv1);
	bundle_free_exported_argv(argc2, &argv2);*/

	return true;
}

void
PeriodicSyncJob::Reset(int accountId, bundle* pUserData, int syncOption, long frequency)
{
	SyncJob::Reset(accountId, pUserData, syncOption);
	__period = frequency;
}

//}//_SyncManager
