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
 * @file	SyncManager_HistoryItem.cpp
 * @brief	This is the implementation file for the HistoryItem class.
 */

#include "SyncManager_HistoryItem.h"

/*namespace _SyncManager
{*/

HistoryItem::HistoryItem(void)
	: __authorityId(0)
	, __historyId(0)
	, __eventTime(0)
	, __elapsedTime(0)
	, __source(0)
	, __event(0)
	, __upstreamActivity(0)
	, __downstreamActivity(0)
	, __msg("")
	, __isInitialized(true)
	, __pExtras(NULL)
	, __reason(0)
{
	//Empty
}


HistoryItem::~HistoryItem(void)
{
	//Empty
}


int
HistoryItem::GetAuthorityId(void) const
{
	return __authorityId;
}


int
HistoryItem::GetHistoryId(void) const
{
	return __historyId;
}


long
HistoryItem::GetEventTime(void) const
{
	return __eventTime;
}


long
HistoryItem::GetElapsedTime(void) const
{
	return __elapsedTime;
}


int
HistoryItem::GetSource(void) const
{
	return __source;
}


int
HistoryItem::GetEvent(void) const
{
	return __event;
}


long
HistoryItem::GetUpstreamActivity(void) const
{
	return __upstreamActivity;
}


long
HistoryItem::GetDownstreamActivity(void) const
{
	return __downstreamActivity;
}


string
HistoryItem::GetMsg(void) const
{
	return __msg;
}


bool
HistoryItem::IsInitialized(void) const
{
	return __isInitialized;
}


map<string, string>*
HistoryItem::GetExtras(void) const
{
	return __pExtras;
}


int
HistoryItem::GetReason(void) const
{
	return __reason;
}


void
HistoryItem::SetAuthorityId(int id)
{
	__authorityId = id;
}


void
HistoryItem::SetHistoryId(int id)
{
	__historyId = id;
}


void
HistoryItem::SetEventTime(int time)
{
	__eventTime = time;
}


void
HistoryItem::SetElapsedTime(int time)
{
	__elapsedTime = time;
}


void
HistoryItem::SetSource(int src)
{
	__source = src;
}


void
HistoryItem::SetEvent(int event)
{
	__event = event;
}


void
HistoryItem::SetUpstreamActivity(long activity)
{
	__upstreamActivity = activity;
}


void
HistoryItem::SetDownstreamActivity(long activity)
{
	__downstreamActivity = activity;
}


void
HistoryItem::SetMsg(string msg)
{
	__msg = msg;
}


void
HistoryItem::SetInitialized(bool isInitialized)
{
	__isInitialized = isInitialized;
}


void
HistoryItem::SetReason(int reason)
{
	__reason = reason;
}


void
HistoryItem::SetExtras(map<string, int>* data)
{
	//__pExtras = data;
}
//}//_SyncManager
