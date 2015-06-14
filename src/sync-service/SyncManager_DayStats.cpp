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
 * @file	SyncManager_DayStats.cpp
 * @brief	This is the implementation file for the DayStats class.
 */

#include <iostream>
#include "SyncManager_DayStats.h"

/*namespace _SyncManager
{*/


DayStats::DayStats(void)
	: __day(0)
	, __successCount(0)
	, __successTime(0)
	, __failureCount(0)
	, __failureTime(0)
{
	//Empty
}


DayStats::DayStats(int dayData)
	: __successCount(0)
	, __successTime(0)
	, __failureCount(0)
	, __failureTime(0)
{
	__day = dayData;
}


DayStats::~DayStats(void)
{
	//Empty
}


int DayStats::GetDay(void) const
{
	return __day;
}


int DayStats::GetSuccessCount(void) const
{
	return __successCount;
}


long DayStats::GetSuccessTime(void) const
{
	return __successTime;
}


int DayStats::GetFailureCount(void) const
{
	return __failureCount;
}


long DayStats::GetFailureTime(void) const
{
	return __failureTime;
}


void DayStats::SetSuccessCount(int count)
{
	__successCount = count;
}


void DayStats::SetDay(int days)
{
	__day = days;
}


void DayStats::SetSuccessTime(long time)
{
	__successTime = time;
}


void DayStats::SetFailureCount(int count)
{
	__failureCount = count;
}


void DayStats::SetFailureTime(long time)
{
	__failureTime = time;
}
//}//_SyncManager
