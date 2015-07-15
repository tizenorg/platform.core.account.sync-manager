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
 * @file	SyncManager_JsonParser.cpp
 * @brief	This is the implementation file for the JsonParser class.
 */

#include "SyncManager_JsonParser.h"
#include "sync-log.h"
#include "sync-error.h"
#include <tzplatform_config.h>


/*namespace _SyncManager
{*/

static const gchar* USER_DATA = "user_data";
static const gchar* USER_ID = "user_id";
static const gchar* ACCOUNT_INFO = "account_info";
static const gchar* ACCOUNT_TYPE_ID = "account_type_id";
static const gchar* ACCOUNT = "account";
static const gchar* ACCOUNT_ID = "account_id";
static const gchar* ACCOUNT_ENABLED = "enabled";
static const gchar* ACCOUNT_CAPABILITY = "capability";
static const gchar* ACCOUNT_CAPABILITY_ID = "id";
static const gchar* PERIODIC_SYNC = "periodic_sync";
static const gchar* PERIOD = "period";
static const gchar* FLEX = "flex";

JsonParser::JsonParser()
	: isAccountData(false)
	, mpParser(NULL)
	, mpCapabilityInfo(NULL)
	, mIsCapabilityInfo(false)
	, mIsPeriodicSyncInfo(false)
	, mpPeriodicSync(NULL)
	, mIsPeriodicObject(false)
{
	LOG_LOGD("JsonParser::JsonParser() Starts");

	mpList.clear();
	mpPeriodicSyncList.clear();

	LOG_LOGD("JsonParser::JsonParser() Ends");
}

JsonParser::~JsonParser()
{
	if (mpParser)
	{
		g_object_unref(mpParser);
		mpParser = NULL;
	}
	if (mpCapabilityInfo)
	{
		delete mpCapabilityInfo;
	}
}

int
JsonParser::parseAccountData()
{
	LOG_LOGD("JsonParser::parseAccountData() Starts");

	 JsonNode* pNode;
	 gboolean gRet = TRUE;
	 GError *error = NULL;
//	 gchar* filename = "/opt/usr/account.json";
	 const gchar* filename = tzplatform_mkpath(TZ_SYS_DATA, "sync-manager/account.json");

	 g_type_init();

	 if (mpParser)
	 {
		 g_object_unref(mpParser);
		 mpParser = NULL;
	 }

	 mpParser = json_parser_new();
	 if (mpParser == NULL)
	 {
		 LOG_ERRORD("json_parser_new failed");
		 return SYNC_ERROR_NONE;
	 }

	 gRet = json_parser_load_from_file(mpParser, filename, &error);
	 if (gRet != TRUE)
	 {
		 LOG_ERRORD("Account File load failed %d", gRet);
	     g_error_free(error);
	     g_object_unref(mpParser);
	     mpParser = NULL;
	     return SYNC_ERROR_SYSTEM;
	}

	 pNode = json_parser_get_root(mpParser);

	if (pNode != NULL)
	{
		isAccountData = true;
		return parseJsonNode(pNode);
	}

	LOG_LOGD("JsonParser::parseAccountData() Ends");

	return SYNC_ERROR_SYSTEM;
}

int
JsonParser::parsePendingJobData()
{
	LOG_LOGD("JsonParser::parsePendingJobData() Starts");

	isAccountData = false;

	LOG_LOGD("JsonParser::parsePendingJobData() Ends");

	return SYNC_ERROR_NONE;
}

void
JsonParser::dumpAccountData()
{
	LOG_LOGD("JsonParser::dumpAccountData() Starts");

	int count = 0;
	for (std::vector<CapabilityInfo>::iterator it = mpList.begin(); it != mpList.end(); ++it)
	{
		int sum = 0;
		CapabilityInfo temp = *it;
		LOG_LOGD("[%d] AccountData::%d %d %d %d %s", count, temp.mAccountId, temp.mAccountTypeId, temp.mUserId, temp.mEnabled, temp.mCapability.c_str());
		for (std::vector<PeriodicSync>::iterator it1 = temp.mPeriodicSyncList.begin(); it1 != temp.mPeriodicSyncList.end(); ++it1)
		{
			PeriodicSync syncTemp = *it1;
			LOG_LOGD("[%d] PeriodicSyncData::%d %d", sum, syncTemp.mPeriod, syncTemp.mFlex);
			++sum;
		}
		++count;
	}

	LOG_LOGD("JsonParser::dumpAccountData() Ends");
}

void
JsonParser::onJsonArrayParsed(JsonArray* pArray, guint index, JsonNode* element_node, gpointer user_data)
{
	LOG_LOGD("JsonParser::parseJsonArray() Starts");

	JsonParser* pJsonParser = static_cast<JsonParser*>(user_data);
	pJsonParser->parseJsonNode(element_node);

	if (pJsonParser != NULL)
	{
		if (pJsonParser->mpCapabilityInfo != NULL)
		{
			if (pJsonParser->mIsCapabilityInfo)
			{
				LOG_LOGD("JsonParser::parseJsonArray() adding capability info data");
				pJsonParser->mpCapabilityInfo->mUserId = pJsonParser->mUserId;
				pJsonParser->mpCapabilityInfo->mAccountTypeId = pJsonParser->mAccountTypeId;

				std::vector<PeriodicSync>::iterator it;
				for (it = pJsonParser->mpPeriodicSyncList.begin(); it != pJsonParser->mpPeriodicSyncList.end(); ++it)
				{
					LOG_LOGD("JsonParser::parseJsonArray() adding periodic sync data to capability list");
					pJsonParser->mpCapabilityInfo->addPeriodicSync(*it);
				}

				pJsonParser->mpPeriodicSyncList.clear();
				pJsonParser->mpList.push_back(*pJsonParser->mpCapabilityInfo);

				delete pJsonParser->mpCapabilityInfo;
				pJsonParser->mpCapabilityInfo = NULL;
				pJsonParser->mIsCapabilityInfo = false;
			}
		}

	}

	LOG_LOGD("JsonParser::parseJsonArray() Ends");
}

void
JsonParser::onJsonPeriodicSyncArrayParsed(JsonArray* pArray, guint index, JsonNode* element_node, gpointer user_data)
{
	LOG_LOGD("JsonParser::onJsonPeriodicSyncArrayParsed() Starts");

	JsonParser* pJsonParser = static_cast<JsonParser*>(user_data);
	pJsonParser->parseJsonNode(element_node);

	if (pJsonParser != NULL)
	{
		if (pJsonParser->mpCapabilityInfo != NULL)
		{
			if (pJsonParser->mIsPeriodicSyncInfo)
			{
				LOG_LOGD("JsonParser::onJsonPeriodicSyncArrayParsed() adding periodic sync data");
				pJsonParser->mpPeriodicSyncList.push_back(*pJsonParser->mpPeriodicSync);

				delete pJsonParser->mpPeriodicSync;
				pJsonParser->mpPeriodicSync = NULL;
				pJsonParser->mIsPeriodicSyncInfo = false;
				pJsonParser->mIsPeriodicObject = false;
			}
		}

	}

	LOG_LOGD("JsonParser::onJsonPeriodicSyncArrayParsed() Ends");
}
int
JsonParser::parseJsonNode(JsonNode* pNode)
{
	LOG_LOGD("JsonParser::parseJsonNode() Starts");

	gint64 value;
	std::string data;
	const gchar * str;

	if (pNode != NULL)
	{
		JsonNodeType type = json_node_get_node_type(pNode);
		switch (type)
		{
			case JSON_NODE_OBJECT:
				{
					JsonObject* pObject = json_node_get_object(pNode);
					if (pObject != NULL)
						return parseJsonObject(pObject);
				}
				break;
			case JSON_NODE_ARRAY:
				{
					JsonArray* pArray = json_node_get_array(pNode);
					if (pArray != NULL)
						return parseJsonArray(pArray);
				}
				break;
			case JSON_NODE_VALUE:
				{
					switch (json_node_get_value_type(pNode))
					{
						case G_TYPE_INT64:
							{
								value = json_node_get_int (pNode);
								LOG_LOGD("JsonParser::parseJsonNode() Integer value is = %d", value);
								setParsedData(value);
							}
							break;
						case G_TYPE_STRING:
							str = json_node_get_string(pNode);
							LOG_LOGD("JsonParser::parseJsonNode() String value is = %s", str);
							data.clear();
							data.append(str);
							setParsedData(data);
							break;
						default:
							break;
					}
					return	SYNC_ERROR_NONE;
				}
				break;
			default:
				break;
		}
	}

	LOG_LOGD("JsonParser::parseJsonNode() Ends");

	return SYNC_ERROR_SYSTEM;
}

int
JsonParser::parseJsonObject(JsonObject* pObject)
{
	LOG_LOGD("JsonParser::parseJsonObject() Starts");

	if (pObject != NULL)
	{
		if (isAccountData)
		{
			JsonNode* pNode;
			pNode = json_object_get_member(pObject, USER_DATA);
			if (pNode != NULL)
			{
				LOG_LOGD("JsonParser::User Data Parsing");
				return parseJsonNode(pNode);
			}
			else
			{
				pNode = json_object_get_member(pObject, USER_ID);
				if (pNode != NULL)
				{
					LOG_LOGD("JsonParser::UserId Parsing");
					mUserId = -1;
					mAccountTypeId = -1;
					key.clear();
					key.append(USER_ID);
					if (parseJsonNode(pNode) == SYNC_ERROR_NONE)
					{
						LOG_LOGD("JsonParser::Account Info Parsing");
						pNode = json_object_get_member(pObject, ACCOUNT_INFO);
						if (pNode != NULL)
							{
								return parseJsonNode(pNode);
							}
					}
				}
				pNode = json_object_get_member(pObject, ACCOUNT_TYPE_ID);
				if (pNode != NULL)
				{
					mAccountTypeId = -1;
					key.clear();
					key.append(ACCOUNT_TYPE_ID);
					if (parseJsonNode(pNode) == SYNC_ERROR_NONE)
					{
						LOG_LOGD("JsonParser::Account details Parsing");
						pNode = json_object_get_member(pObject, ACCOUNT);
						if (pNode != NULL)
						{
							return parseJsonNode(pNode);
						}
					}
				}
				pNode = json_object_get_member(pObject, ACCOUNT_ID);
				if (pNode != NULL)
				{
					key.clear();
					key.append(ACCOUNT_ID);
					if (mpCapabilityInfo)
					{
						delete mpCapabilityInfo;
						mpCapabilityInfo = NULL;
					}
					mpCapabilityInfo = new CapabilityInfo();
                    if (mpCapabilityInfo == NULL)
                    {
                        LOG_LOGD("heap error");
                    }
					mIsCapabilityInfo = true;
					if (parseJsonNode(pNode) == SYNC_ERROR_NONE)
					{
						LOG_LOGD("JsonParser::Sync enabled details");
						pNode = json_object_get_member(pObject, ACCOUNT_ENABLED);
						key.clear();
						key.append(ACCOUNT_ENABLED);
						if (pNode != NULL && parseJsonNode(pNode) == SYNC_ERROR_NONE)
						{
							LOG_LOGD("JsonParser::Parsing capability");
							pNode = json_object_get_member(pObject, ACCOUNT_CAPABILITY);
							return parseJsonNode(pNode);
						}
					}
				}
				pNode = json_object_get_member(pObject, ACCOUNT_CAPABILITY_ID);
				if (pNode != NULL)
				{
					key.clear();
					key.append(ACCOUNT_CAPABILITY_ID);
					if (parseJsonNode(pNode) == SYNC_ERROR_NONE)
					{
						LOG_LOGD("JsonParser::Periodic Sync details");
						mIsPeriodicObject = true;
						pNode = json_object_get_member(pObject, PERIODIC_SYNC);
						return parseJsonNode(pNode);
					}
				}
				pNode = json_object_get_member(pObject, PERIOD);
				if (pNode != NULL)
				{
					key.clear();
					key.append(PERIOD);
					mIsPeriodicSyncInfo = true;
					if (mpPeriodicSync)
					{
						delete mpPeriodicSync;
						mpPeriodicSync = NULL;
					}
					mpPeriodicSync = new PeriodicSync();
                    if (mpPeriodicSync == NULL)
                    {
                        LOG_LOGD("heap error");
                    }
					if (parseJsonNode(pNode) == SYNC_ERROR_NONE)
					{
						LOG_LOGD("JsonParser:: Flex details");
						pNode = json_object_get_member(pObject, FLEX);
						key.clear();
						key.append(FLEX);
						return parseJsonNode(pNode);
					}
				}
			}
		}
	}

	LOG_LOGD("JsonParser::parseJsonObject() Ends");

	return SYNC_ERROR_SYSTEM;
}

int
JsonParser::parseJsonArray(JsonArray* pArray)
{
	LOG_LOGD("JsonParser::parseJsonArray() Starts");

	if (pArray != NULL)
	{
		if (mIsPeriodicObject)
		{
			LOG_LOGD("JsonParser::parseJsonArray() For periodic sync");
			json_array_foreach_element(pArray,JsonParser::onJsonPeriodicSyncArrayParsed,this);
		}
		else
		{
			json_array_foreach_element(pArray,JsonParser::onJsonArrayParsed,this);
		}
		return SYNC_ERROR_NONE;
	}

	LOG_LOGD("JsonParser::parseJsonArray() Ends");

	return SYNC_ERROR_SYSTEM;
}

void
JsonParser::setParsedData(int value)
{
	LOG_LOGD("JsonParser::setParsedData() Starts");

	if (!key.compare(USER_ID))
	{
		LOG_LOGD("JsonParser::setParsedData() user id");
		mUserId = value;
	}
	else if (!key.compare(ACCOUNT_TYPE_ID))
	{
		LOG_LOGD("JsonParser::setParsedData() account type id");
		mAccountTypeId = value;
	}
	else if (!key.compare(ACCOUNT_ENABLED))
	{
		LOG_LOGD("JsonParser::setParsedData() account enabled");
		if (value)
		{
			mpCapabilityInfo->mEnabled = true;
		}
		else
		{
			mpCapabilityInfo->mEnabled = false;
		}
	}
	else if (!key.compare(ACCOUNT_ID))
	{
		LOG_LOGD("JsonParser::setParsedData() account id");
		if (mpCapabilityInfo)
		{
			mpCapabilityInfo->mAccountId = value;
		}
	}
	else if (!key.compare(PERIOD))
	{
		LOG_LOGD("JsonParser::set for period");
		if (mpPeriodicSync)
		{
			mpPeriodicSync->mPeriod = value;
		}
	}
	else if (!key.compare(FLEX))
	{
		LOG_LOGD("JsonParser::set for flex");
		if (mpPeriodicSync)
		{
			mpPeriodicSync->mFlex = value;
		}
	}
	LOG_LOGD("JsonParser::setParsedData() Ends");
}

void
JsonParser::setParsedData(std::string keyValue)
{
	LOG_LOGD("JsonParser::setParsedData() Starts");

	if (!key.compare(ACCOUNT_CAPABILITY_ID))
	{
		if (mpCapabilityInfo)
		{
			mpCapabilityInfo->mCapability.clear();
			mpCapabilityInfo->mCapability.append(keyValue);
		}
	}
	else
	{
		return;
	}

	LOG_LOGD("JsonParser::setParsedData() Ends");
}
//}//_SyncManager
