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
 * @file    SyncManager_DataChangeListener.cpp
 * @brief   This is the implementation file for the DataChangeListener class.
 */


#include <calendar.h>
#include <contacts.h>
#include "sync-error.h"
#include "SyncManager_DataChangeListener.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncManager.h"


void OnCalendarBookChanged(const char* view_uri, void* user_data)
{
    LOG_LOGD("Invoked CalendarDataChanged");

    DataChangeStatus value = CALENDAR_BOOK_CHANGED;
    SyncManager::GetInstance()->OnCalendarDataChanged(value);
}


void OnCalendarEventChanged(const char* view_uri, void* user_data)
{
    LOG_LOGD("Invoked CalendarDataChanged");

    DataChangeStatus value = CALENDAR_EVENT_CHANGED;
    SyncManager::GetInstance()->OnCalendarDataChanged(value);
}


void OnCalendarTodoChanged(const char* view_uri, void* user_data)
{
    LOG_LOGD("Invoked CalendarDataChanged");

    DataChangeStatus value = CALENDAR_TODO_CHANGED;
    SyncManager::GetInstance()->OnCalendarDataChanged(value);
}


void OnContactsDataChanged(const char* view_uri, void* user_data)
{
    LOG_LOGD("Invoked ContactsDataChanged");

    DataChangeStatus value = CONTACTS_DATA_CHANGED;
    SyncManager::GetInstance()->OnContactsDataChanged(value);
}


DataChangeListener::DataChangeListener(void)
{
    calendar_subscription_started = false;
    contacts_subscription_started = false;
}


DataChangeListener::~DataChangeListener(void)
{

}


int DataChangeListener::SubscribeCalendarCallback(void)
{
    SYNC_LOGE_RET_RES(!calendar_subscription_started, SYNC_ERROR_NONE, "Calendar Callback Already Subscribed");
    int err = VALUE_UNDEFINED;

    err = calendar_connect_with_flags(CALENDAR_CONNECT_FLAG_RETRY);
    SYNC_LOGE_RET_RES(err == CALENDAR_ERROR_NONE, SYNC_ERROR_INVALID_OPERATION, "Calendar Connection Failed");

    err = calendar_db_add_changed_cb(_calendar_book._uri, OnCalendarBookChanged, this);
    if (err != CALENDAR_ERROR_NONE) {
        calendar_disconnect();

        LOG_LOGD("Subscribing Calendar Callback for BOOK Failed");

        return SYNC_ERROR_INVALID_OPERATION;
    }

    err = calendar_db_add_changed_cb(_calendar_event._uri, OnCalendarEventChanged, this);
    if (err != CALENDAR_ERROR_NONE) {
        calendar_db_remove_changed_cb(_calendar_book._uri, OnCalendarBookChanged, NULL);
        calendar_disconnect();

        LOG_LOGD("Subscribing Calendar Callback for EVENT Failed");

        return SYNC_ERROR_INVALID_OPERATION;
    }

    err = calendar_db_add_changed_cb(_calendar_todo._uri, OnCalendarTodoChanged, this);
    if (err != CALENDAR_ERROR_NONE) {
        calendar_db_remove_changed_cb(_calendar_book._uri, OnCalendarBookChanged, NULL);
        calendar_db_remove_changed_cb(_calendar_event._uri, OnCalendarEventChanged, NULL);
        calendar_disconnect();

        LOG_LOGD("Subscribing Calendar Callback for TODO Failed");

        return SYNC_ERROR_INVALID_OPERATION;
    }

    calendar_subscription_started = true;

    return SYNC_ERROR_NONE;
}


int DataChangeListener::SubscribeContactsCallback(void)
{
    SYNC_LOGE_RET_RES(!contacts_subscription_started, SYNC_ERROR_NONE, "Contacts Callback Already Subscribed");
    int err = VALUE_UNDEFINED;

    err = contacts_connect_with_flags(CONTACTS_CONNECT_FLAG_RETRY);
    SYNC_LOGE_RET_RES(err == CONTACTS_ERROR_NONE, SYNC_ERROR_INVALID_OPERATION, "Contacts Connection Failed");

    err = contacts_db_add_changed_cb(_contacts_contact._uri, OnContactsDataChanged, this);
    if (err != CONTACTS_ERROR_NONE) {
        contacts_disconnect();

        LOG_LOGD("Subscribing Contacts Callback for DataChanged Failed");

        return SYNC_ERROR_INVALID_OPERATION;
    }

    contacts_subscription_started = true;

    return SYNC_ERROR_NONE;
}


int DataChangeListener::UnSubscribeCalendarCallback(void)
{
    SYNC_LOGE_RET_RES(calendar_subscription_started, SYNC_ERROR_NONE, "Calendar Callback Already UnSubscribed");

    calendar_db_remove_changed_cb(_calendar_book._uri, OnCalendarBookChanged, NULL);
    calendar_db_remove_changed_cb(_calendar_event._uri, OnCalendarEventChanged, NULL);
    calendar_db_remove_changed_cb(_calendar_todo._uri, OnCalendarTodoChanged, NULL);
    calendar_disconnect();

    calendar_subscription_started = false;

    return SYNC_ERROR_NONE;
}


int DataChangeListener::UnSubscribeContactsCallback(void)
{
    SYNC_LOGE_RET_RES(contacts_subscription_started, SYNC_ERROR_NONE, "Contacts Callback Already UnSubscribed");

    contacts_db_remove_changed_cb(_contacts_contact._uri, OnContactsDataChanged, NULL);
    contacts_disconnect();

    contacts_subscription_started = false;

    return SYNC_ERROR_NONE;
}


int DataChangeListener::RegisterDataChangeListener(void)
{
    int err = VALUE_UNDEFINED;

    err = DataChangeListener::SubscribeCalendarCallback();
    if (err != SYNC_ERROR_NONE) {
        LOG_LOGD("Registration of Calendar DataChangeListener Failed");
        return SYNC_ERROR_INVALID_OPERATION;
    }

    err = DataChangeListener::SubscribeContactsCallback();
    if (err != SYNC_ERROR_NONE) {
        LOG_LOGD("Registration of Contacts DataChangeListener Failed");
        return SYNC_ERROR_INVALID_OPERATION;
    }

    return SYNC_ERROR_NONE;
}


int DataChangeListener::DeRegisterDataChangeListener(void)
{
    int err = VALUE_UNDEFINED;

    err = DataChangeListener::UnSubscribeCalendarCallback();
    if (err != SYNC_ERROR_NONE) {
        LOG_LOGD("DeRegistration of Calendar DataChangeListener Failed");
        return SYNC_ERROR_INVALID_OPERATION;
    }

    err = DataChangeListener::UnSubscribeContactsCallback();
    if (err != SYNC_ERROR_NONE) {
        LOG_LOGD("DeRegistration of Contacts DataChangeListener Failed");
        return SYNC_ERROR_INVALID_OPERATION;
    }

    LOG_LOGD("DeRegistration of DataChangeListener Successfully Ends");

    return SYNC_ERROR_NONE;
}
