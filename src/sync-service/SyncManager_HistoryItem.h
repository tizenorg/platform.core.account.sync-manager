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
 * @file	SyncManager_HistoryItem.h
 * @brief	This is the header file for the HistoryItem class.
 */

#ifndef SYNC_SERVICE_HISTORY_ITEM_H
#define SYNC_SERVICE_HISTORY_ITEM_H

#include <iostream>
#include <string>
#include <map>

/*namespace _SyncManager
{
*/

using namespace std;

class HistoryItem
{
public:
	HistoryItem(void);

	~HistoryItem(void);

	int GetAuthorityId(void) const;

	int GetHistoryId(void) const;

	long GetEventTime(void) const;

	long GetElapsedTime(void) const;

	int GetSource(void) const;

	int GetEvent(void) const;

	long GetUpstreamActivity(void) const;

	long GetDownstreamActivity(void) const;

	string GetMsg(void) const;

	bool IsInitialized(void) const;

	map<string, string>* GetExtras(void) const;

	int GetReason(void) const;

	void SetAuthorityId(int id);

	void SetHistoryId(int id);

	void SetEventTime(int time);

	void SetElapsedTime(int time);

	void SetSource(int src);

	void SetEvent(int event);

	void SetUpstreamActivity(long activity);

	void SetDownstreamActivity(long activity);

	void SetMsg(string msg);

	void SetInitialized(bool isInitialized);

	void SetReason(int reason);

	void SetExtras(map<string, int>* data);

private:
	int __authorityId;
	int __historyId;
	long __eventTime;
	long __elapsedTime;
	int __source;
	int __event;
	long __upstreamActivity;
	long __downstreamActivity;
	string __msg;
	bool __isInitialized;
	map<string, string>* __pExtras;
	int __reason;
};
//}//_SyncManager
#endif // SYNC_SERVICE_HISTORY_ITEM_H
