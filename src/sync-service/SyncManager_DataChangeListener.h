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
 * @file    SyncManager_DataChangeListener.h
 * @brief   This is the header file for the DataChangeListener class.
 */


#ifndef _SYNC_SERVICE_DATA_CHANGED_LISTENER_H_
#define _SYNC_SERVICE_DATA_CHANGED_LISTENER_H_


class DataChangeListener
{
    public:
        bool calendar_subscription_started;
        bool contacts_subscription_started;

    public:
        DataChangeListener(void);
        ~DataChangeListener(void);

        int SubscribeCalendarCallback(void);
        int SubscribeContactsCallback(void);

        int UnSubscribeCalendarCallback(void);
        int UnSubscribeContactsCallback(void);

        int RegisterDataChangeListener(void);
        int DeRegisterDataChangeListener(void);
};


#endif // _SYNC_SERVICE_DATA_CHANGED_LISTENER_H_