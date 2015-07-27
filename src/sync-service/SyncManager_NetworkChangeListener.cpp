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
 * @file	SyncManager_NetworkChangeListener.cpp
 * @brief	This is the implementation file for the NetworkChangeListener class.
 */


#include "SyncManager_SyncManager.h"
#include "SyncManager_NetworkChangeListener.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{*/


void OnConnectionChanged(connection_type_e type, void *user_data)
{
	LOG_LOGD("Network connection changed %d", type);

	switch (type)
	{
		case CONNECTION_TYPE_WIFI:
		{
			SyncManager::GetInstance()->OnWifiStatusChanged(true);
			break;
		}
		case CONNECTION_TYPE_CELLULAR:
		{
			SyncManager::GetInstance()->OnDNetStatusChanged(true);
			break;
		}
		case CONNECTION_TYPE_BT:
		{
			SyncManager::GetInstance()->OnBluetoothStatusChanged(true);
			break;
		}
		case CONNECTION_TYPE_DISCONNECTED:
		{
			SyncManager::GetInstance()->OnWifiStatusChanged(false);
			SyncManager::GetInstance()->OnDNetStatusChanged(false);
			SyncManager::GetInstance()->OnBluetoothStatusChanged(false);
			break;
		}
		default:
			break;
	}
}


NetworkChangeListener::NetworkChangeListener(void)
	: connection(NULL)
{
	int ret = connection_create(&connection);
	if (ret != CONNECTION_ERROR_NONE)
	{
		LOG_LOGD("Create connection failed %d, %s", ret, get_error_message(ret));
	}
}


NetworkChangeListener::~NetworkChangeListener(void)
{
	if (connection)
	{
		connection_destroy(connection);
	}
}


bool
NetworkChangeListener::IsWifiConnected()
{
	int ret;
	connection_wifi_state_e state = CONNECTION_WIFI_STATE_DEACTIVATED;
	ret = connection_get_wifi_state(connection, &state);
	if (ret != CONNECTION_ERROR_NONE)
	{
		LOG_LOGD("Connection wifi failure %d, %s", ret, get_error_message(ret));
	}
	return (state == CONNECTION_WIFI_STATE_CONNECTED);
}


bool
NetworkChangeListener::IsDataConnectionPresent()
{
	int ret;
	connection_cellular_state_e state = CONNECTION_CELLULAR_STATE_OUT_OF_SERVICE;
	ret = connection_get_cellular_state(connection, &state);
	if (ret != CONNECTION_ERROR_NONE)
	{
		LOG_LOGD("Connection cellular failure %d, %s", ret, get_error_message(ret));
	}
	return (state == CONNECTION_CELLULAR_STATE_CONNECTED);
}


int
NetworkChangeListener::RegisterNetworkChangeListener(void)
{
	int ret = 0;
	ret =  connection_set_type_changed_cb(connection, OnConnectionChanged, NULL);
	if (ret != CONNECTION_ERROR_NONE)
	{
		LOG_LOGD("Registration of network change listener failed %d, %s", ret, get_error_message(ret));
	}
	return ret;
}


int
NetworkChangeListener::DeRegisterNetworkChangeListener(void)
{
	LOG_LOGD("Removing network change listener");

	int ret;
	ret = connection_unset_type_changed_cb(connection);
	if (ret != CONNECTION_ERROR_NONE)
	{
		LOG_LOGD("Removal of network change listener failed");
	}

	return ret;
}

//}//_SyncManager
