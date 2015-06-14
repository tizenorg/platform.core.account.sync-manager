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
 * @file	SyncManager_NetworkChangeListener.h
 * @brief	This is the header file for the NetworkChangeListener class.
 */


#ifndef SYNC_SERVICE_NETWORK_CHANGE_LISTENER_H
#define SYNC_SERVICE_NETWORK_CHANGE_LISTENER_H

#include <net_connection.h>

/*namespace _SyncManager
{
*/

class NetworkChangeListener
{

public:
	NetworkChangeListener(void);

	~NetworkChangeListener(void);

	int RegisterNetworkChangeListener(void);

	bool IsWifiConnected();

	bool IsDataConnectionPresent();

	int DeRegisterNetworkChangeListener(void);
private:
	connection_h connection;
};
//}//_SyncManager
#endif // SYNC_SERVICE_NETWORK_CHANGE_LISTENER_H
