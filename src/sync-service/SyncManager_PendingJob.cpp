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
 * @file	SyncManager_PendingJob.cpp
 * @brief	This is the implementation file for the PendingJob class.
 */

#include <iostream>
#include "SyncManager_PendingJob.h"


/*namespace _SyncManager
{*/


PendingJob::PendingJob(void)
	: appId(NULL)
	, account(NULL)
	, reason(0)
	, syncSource(0)
	, capability("")
	, pExtras(NULL)
	, isExpedited(false)
	, capabilityId(0)
{
	//Empty
}


PendingJob::~PendingJob(void)
{
	if (pExtras)
	{
		bundle_free(pExtras);
	}
}


PendingJob::PendingJob(string appId, account_h account, int reason, int source, string capability,
					   bundle* pExtra, bool isExpedited)
{
	this->appId = appId;
	this->account = account;
	this->reason = reason;
	this->syncSource = source;
	this->capability = capability;
	this->pExtras = bundle_dup(pExtra);
	this->isExpedited = isExpedited;
	this->capabilityId = -1;
}


PendingJob::PendingJob(const PendingJob& otherJob)
{
	this->appId = otherJob.appId;
	this->account = otherJob.account;
	this->reason = otherJob.reason;
	this->syncSource = otherJob.syncSource;
	this->capability = otherJob.capability;
	this->pExtras = bundle_dup(otherJob.pExtras);
	this->isExpedited = otherJob.isExpedited;
	this->capabilityId = otherJob.capabilityId;
}
//}//_SyncManager
