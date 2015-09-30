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
 * @file	SyncManager_CurrentSyncJobQueue.cpp
 * @brief	This is the implementation file for the CurrentSyncJobQueue class.
 */

#include <iostream>
#include <sstream>
#include <alarm.h>
#include <glib.h>
#include "sync-log.h"
#include "sync-error.h"
#include "SyncManager_SyncManager.h"
#include "SyncManager_CurrentSyncJobQueue.h"

using namespace std;
/*namespace _SyncManager
{*/


CurrentSyncJobQueue::CurrentSyncJobQueue(void)
{
	//Empty
}


CurrentSyncJobQueue::~CurrentSyncJobQueue(void)
{
	//Empty
}


int
CurrentSyncJobQueue::AddSyncJobToCurrentSyncQueue(SyncJob* syncJob)
{
	LOG_LOGD("Active Sync Jobs Queue size : Before = %d", __currentSyncJobQueue.size());

	map<const string, CurrentSyncContext*>::iterator it;
	it = __currentSyncJobQueue.find(syncJob->__key);

	if (it != __currentSyncJobQueue.end())
	{
		LOG_LOGD("Sync already in progress");
		return SYNC_ERROR_ALREADY_IN_PROGRESS;
	}
	else
	{
		LOG_LOGD("Create a Sync context");

		CurrentSyncContext* pCurrentSyncContext = new (std::nothrow) CurrentSyncContext(syncJob);
		if (!pCurrentSyncContext)
		{
			LOG_LOGD("Failed to construct CurrentSyncContext instance");
			return SYNC_ERROR_OUT_OF_MEMORY;
		}
		//adding timeout of 30 seconds
		pCurrentSyncContext->SetTimerId(g_timeout_add (300000, CurrentSyncJobQueue::OnTimerExpired, pCurrentSyncContext));
		pair<map<const string, CurrentSyncContext*>::iterator,bool> ret;
		ret = __currentSyncJobQueue.insert(pair<const string, CurrentSyncContext*>(syncJob->__key, pCurrentSyncContext));
		if (ret.second == false)
		{
			 return SYNC_ERROR_ALREADY_IN_PROGRESS;
		}
	}
	LOG_LOGD("Active Sync Jobs Queue size : After = %d", __currentSyncJobQueue.size());

	return SYNC_ERROR_NONE;
}


int
CurrentSyncJobQueue::OnTimerExpired(void* data)
{
	LOG_LOGD("CurrentSyncJobQueue::onTimerExpired Starts");

	CurrentSyncContext* pSyncContext = static_cast<CurrentSyncContext*>(data);
	if (pSyncContext)
	{
		LOG_LOGD("CurrentSyncJobQueue::onTimerExpired sending sync-cancelled message");
		SyncJob* pJob = pSyncContext->GetSyncJob();
		if (pJob)
		{
			SyncManager::GetInstance()->CloseCurrentSyncContext(pSyncContext);
			SyncManager::GetInstance()->SendSyncCompletedOrCancelledMessage(pJob, SYNC_STATUS_CANCELLED);
			LOG_LOGD("CurrentSyncJobQueue::onTimerExpired sending sync-cancelled message end");
		}
		else
		{
			LOG_LOGD("Failed to get SyncJob");
		}
	}
	else
	{
		LOG_LOGD(" context null");
	}

	LOG_LOGD("CurrentSyncJobQueue::onTimerExpired Ends");

	return false;
}


list<CurrentSyncContext*>
CurrentSyncJobQueue::GetOperations(void)
{
	list<CurrentSyncContext*> opsList;
	map<const string, CurrentSyncContext*>::iterator it;
	for (it = __currentSyncJobQueue.begin(); it != __currentSyncJobQueue.end(); it++)
	{
		CurrentSyncContext *pContext = new CurrentSyncContext(*(it->second));
		opsList.push_back(pContext);
	}
	return opsList;
}


bool
CurrentSyncJobQueue::IsJobActive(CurrentSyncContext *pCurrSync)
{
	LOG_LOGD("CurrentSyncJobQueue::IsJobActive() Starts");

	LOG_LOGD("job q size %d", __currentSyncJobQueue.size());

	if (pCurrSync == NULL)
	{
		return false;
	}

	string jobKey;
	jobKey.append(pCurrSync->GetSyncJob()->__key);

	map<const string, CurrentSyncContext*>::iterator it;
	it = __currentSyncJobQueue.find(jobKey);

	if (it != __currentSyncJobQueue.end())
	{
		LOG_LOGD("job active");
		return true;
	}
	LOG_LOGD("job in-active");
	return false;
	LOG_LOGD("CurrentSyncJobQueue::IsJobActive() Ends");
}

int
CurrentSyncJobQueue::RemoveSyncContextFromCurrentSyncQueue(CurrentSyncContext* pSyncContext)
{
	 LOG_LOGD("Remove sync job from Active Sync Jobs queue");
	if (pSyncContext == NULL)
	{
		LOG_LOGD("sync cotext is null");
		return SYNC_ERROR_INVALID_PARAMETER;
	}

	SyncJob* pSyncJob = pSyncContext->GetSyncJob();

	map<const string, CurrentSyncContext*>::iterator it = __currentSyncJobQueue.find(pSyncJob->__key);
	CurrentSyncContext* pCurrContext = it->second;
	__currentSyncJobQueue.erase(it);
	LOG_LOGD("Active Sync Jobs queue size, After = %d", __currentSyncJobQueue.size());
	delete pCurrContext;
	pCurrContext = NULL;
	return SYNC_ERROR_NONE;
}

string
CurrentSyncJobQueue::ToKey(account_h account, string capability)
{
	int ret = ACCOUNT_ERROR_NONE;
	string key;
	char* pName;
	int id;
	stringstream ss;

	ret = account_get_user_name(account, &pName);
	if (ret != ACCOUNT_ERROR_NONE)
		LOG_LOGD("Account get user name failed because of [%s]", get_error_message(ret));

	ret = account_get_account_id(account, &id);
	if (ret != ACCOUNT_ERROR_NONE)
		LOG_LOGD("Account get account id failed because of [%s]", get_error_message(ret));

	ss<<id;
	key.append("id:").append(ss.str()).append("name:").append(pName).append("capability:").append(capability.c_str());

	return key;
}

CurrentSyncContext*
CurrentSyncJobQueue::DoesAccAuthExist(account_h account, string auth)
{
	if (account == NULL)
	{
		LOG_LOGD("CurrentSyncJobQueue::DoesAccAuthExist account is null");
		return NULL;
	}
	LOG_LOGD("CurrentSyncJobQueue::DoesAccAuthExist() Starts");

	string key = ToKey(account, auth);

	map<const string, CurrentSyncContext*>::iterator it;
	it = __currentSyncJobQueue.find(key);
	if (it == __currentSyncJobQueue.end())
	{
		return NULL;
	}

	LOG_LOGD("CurrentSyncJobQueue::DoesAccAuthExist() Ends");
	return (CurrentSyncContext*)it->second;
}

CurrentSyncContext*
CurrentSyncJobQueue::GetCurrJobfromKey(string key)
{
	map<const string, CurrentSyncContext*>::iterator it;
	it = __currentSyncJobQueue.find(key);
	if (it == __currentSyncJobQueue.end())
	{
		return NULL;
	}
	return (CurrentSyncContext*)it->second;
}

//}//_SyncManager
