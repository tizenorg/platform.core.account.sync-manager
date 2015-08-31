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
 * @file	SyncManager_DayStats.h
 * @brief	This is the header file for the DayStats class.
 */

#ifndef SYNC_SERVICE_DAY_STATS_H
#define SYNC_SERVICE_DAY_STATS_H

/*namespace _SyncManager
{
*/

class DayStats
{
public:
	DayStats(void);

	DayStats(int dayData);

	~DayStats(void);

	int GetDay(void) const;

	int GetSuccessCount(void) const;

	long GetSuccessTime(void) const;

	int GetFailureCount(void) const;

	long GetFailureTime(void) const;

	void SetDay(int days);

	void SetSuccessCount(int count);

	void SetSuccessTime(long time);

	void SetFailureCount(int count);

	void SetFailureTime(long time);

public:
	int __day;
	int __successCount;
	long __successTime;
	int __failureCount;
	long __failureTime;
};

//}//_SyncManager
#endif // SYNC_SERVICE_DAY_STATS_H
