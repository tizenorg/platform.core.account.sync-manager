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
 * @file	SyncManager_JsonParser.h
 * @brief	This is the header file for the JsonParser class.
 */

#ifndef SYNC_SERVICE_JSONPARSER_H
#define SYNC_SERVICE_JSONPARSER_H

#include <json-glib/json-glib.h>
#include <glib-object.h>
#include "SyncManager_CapabilityInfo.h"
#include <vector>


/*namespace _SyncManager
{
*/

using namespace std;

class JsonParser
{
public:
	JsonParser(void);
	~JsonParser(void);
	int parseAccountData(void);
	int parsePendingJobData(void);

	//for Testing
	void dumpAccountData(void);

	static void onJsonArrayParsed(JsonArray* pArray, guint index, JsonNode* element_node, gpointer user_data);
	static void onJsonPeriodicSyncArrayParsed(JsonArray* pArray, guint index, JsonNode* element_node, gpointer user_data);
private:
	JsonParser(const JsonParser& obj);
	JsonParser& operator=(const JsonParser& obj);
private:
	int parseJsonNode(JsonNode* node);
	int parseJsonObject(JsonObject* obj);
	int parseJsonArray(JsonArray* pArray);
	void setParsedData(int value);
	void setParsedData(std::string value);
	void addAccountData(const JsonParser* pJsonParser);

private:
	JsonParser *mpParser;
	bool isAccountData;
	std::string key;
	CapabilityInfo* mpCapabilityInfo;
	PeriodicSync* mpPeriodicSync;
	std::vector<CapabilityInfo> mpList;
	std::vector<PeriodicSync> mpPeriodicSyncList;
	int mUserId;
	int mAccountTypeId;
	bool mIsCapabilityInfo;
	bool mIsPeriodicSyncInfo;
	bool mIsPeriodicObject;
};

//}//_SyncManager
#endif // SYNC_SERVICE_JSONPARSER_H
