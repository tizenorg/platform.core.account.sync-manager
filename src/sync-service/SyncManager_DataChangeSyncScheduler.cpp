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
#include <media_content.h>
#include "sync-error.h"
#include "sync_manager.h"
#include "SyncManager_DataChangeSyncScheduler.h"
#include "SyncManager_SyncDefines.h"
#include "SyncManager_SyncManager.h"


void OnCalendarBookChanged(const char* view_uri, void* user_data)
{
	LOG_LOGD("On Calendar Book Changed");

	DataChangeSyncScheduler* pDCScheduler = (DataChangeSyncScheduler*) (user_data);
	pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_CALENDAR);
}


void OnCalendarEventChanged(const char* view_uri, void* user_data)
{
	LOG_LOGD("On Calendar Event Changed");

	DataChangeSyncScheduler* pDCScheduler = (DataChangeSyncScheduler*) (user_data);
	pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_CALENDAR);
}


void OnCalendarTodoChanged(const char* view_uri, void* user_data)
{
	LOG_LOGD("On Calendar TODO Changed");

	DataChangeSyncScheduler* pDCScheduler = (DataChangeSyncScheduler*) (user_data);
	pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_CALENDAR);
}


void OnContactsDataChanged(const char* view_uri, void* user_data)
{
	LOG_LOGD("On Contacts Data Changed");

	DataChangeSyncScheduler* pDCScheduler = (DataChangeSyncScheduler*) (user_data);
	pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_CONTACT);
}


void OnMediaContentDataChanged(media_content_error_e error, int pid, media_content_db_update_item_type_e update_item, media_content_db_update_type_e update_type, media_content_type_e media_type, char *uuid, char *path, char *mime_type, void *user_data)
{
	LOG_LOGD("On Media Content Data Changed");

	DataChangeSyncScheduler* pDCScheduler = (DataChangeSyncScheduler*) (user_data);

	switch (media_type) {
	case MEDIA_CONTENT_TYPE_IMAGE:
	{
		LOG_LOGD("Media Content Image Type Data Changed");
		pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_IMAGE);
		break;
	}
	case MEDIA_CONTENT_TYPE_VIDEO:
	{
		LOG_LOGD("Media Content Video Type Data Changed");
		pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_VIDEO);
		break;
	}
	case MEDIA_CONTENT_TYPE_MUSIC:
	{
		LOG_LOGD("Media Content Music Type Data Changed");
		pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_MUSIC);
		break;
	}
	case MEDIA_CONTENT_TYPE_SOUND:
	{
		LOG_LOGD("Media Content Sound Type Data Changed");
		pDCScheduler->HandleDataChangeEvent(SYNC_SUPPORTS_CAPABILITY_SOUND);
		break;
	}
	case MEDIA_CONTENT_TYPE_OTHERS:
	{
		break;
	}
	default:
	{
		LOG_LOGD("Invalid capability is inserted");
	}
	}

}


DataChangeSyncScheduler::DataChangeSyncScheduler(void)
{
	calendar_subscription_started = false;
	contacts_subscription_started = false;
	media_content_subscription_started = false;
}


DataChangeSyncScheduler::~DataChangeSyncScheduler(void)
{

}


int
DataChangeSyncScheduler::SubscribeCalendarCallback(void)
{
	SYNC_LOGE_RET_RES(!calendar_subscription_started, SYNC_ERROR_NONE, "Calendar Callback Already Subscribed");
	int err = VALUE_UNDEFINED;

	err = calendar_connect_with_flags(CALENDAR_CONNECT_FLAG_RETRY);

	if (err != CALENDAR_ERROR_NONE)
		LOG_LOGD("Calendar connection failed : %d, %s", err, get_error_message(err));

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


int
DataChangeSyncScheduler::SubscribeContactsCallback(void)
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


int
DataChangeSyncScheduler::SubscribeMediaContentCallback(void)
{
	SYNC_LOGE_RET_RES(!media_content_subscription_started, SYNC_ERROR_NONE, "Media Content Callback Already Subscribed");
	int err = VALUE_UNDEFINED;

	err = media_content_connect();
	if (err == MEDIA_CONTENT_ERROR_DB_FAILED)
		LOG_LOGD("media content connection error [%s]", get_error_message(err));
	SYNC_LOGE_RET_RES(err == MEDIA_CONTENT_ERROR_NONE, SYNC_ERROR_INVALID_OPERATION, "Media Content Connection Failed");

	err = media_content_set_db_updated_cb(OnMediaContentDataChanged, this);
	if (err != MEDIA_CONTENT_ERROR_NONE) {
		media_content_disconnect();

		LOG_LOGD("Subscribing Media Content Callback for DataChanged Failed");

		return SYNC_ERROR_INVALID_OPERATION;
	}

	media_content_subscription_started = true;

	return SYNC_ERROR_NONE;
}


int
DataChangeSyncScheduler::UnSubscribeCalendarCallback(void)
{
	SYNC_LOGE_RET_RES(calendar_subscription_started, SYNC_ERROR_NONE, "Calendar Callback Already UnSubscribed");

	calendar_db_remove_changed_cb(_calendar_book._uri, OnCalendarBookChanged, NULL);
	calendar_db_remove_changed_cb(_calendar_event._uri, OnCalendarEventChanged, NULL);
	calendar_db_remove_changed_cb(_calendar_todo._uri, OnCalendarTodoChanged, NULL);
	calendar_disconnect();

	calendar_subscription_started = false;

	return SYNC_ERROR_NONE;
}


int
DataChangeSyncScheduler::UnSubscribeContactsCallback(void)
{
	SYNC_LOGE_RET_RES(contacts_subscription_started, SYNC_ERROR_NONE, "Contacts Callback Already UnSubscribed");

	contacts_db_remove_changed_cb(_contacts_contact._uri, OnContactsDataChanged, NULL);
	contacts_disconnect();

	contacts_subscription_started = false;

	return SYNC_ERROR_NONE;
}


int
DataChangeSyncScheduler::UnSubscribeMediaContentCallback(void)
{
	SYNC_LOGE_RET_RES(media_content_subscription_started, SYNC_ERROR_NONE, "Media Content Callback Already UnSubscribed");

	media_content_unset_db_updated_cb();
	media_content_disconnect();

	media_content_subscription_started = false;

	return SYNC_ERROR_NONE;
}


int
DataChangeSyncScheduler::RegisterDataChangeListeners(void)
{
	int err = SYNC_ERROR_NONE;

	err = SubscribeCalendarCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("Registration of Calendar DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	err = SubscribeContactsCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("Registration of Contacts DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	err = SubscribeMediaContentCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("Registration of Media Content DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	LOG_LOGD("Registration of DataChangeListener Successfully Ends");

	return SYNC_ERROR_NONE;
}


int
DataChangeSyncScheduler::DeRegisterDataChangeListeners(void)
{
	int err = VALUE_UNDEFINED;

	err = UnSubscribeCalendarCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("DeRegistration of Calendar DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	err = UnSubscribeContactsCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("DeRegistration of Contacts DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	err = UnSubscribeMediaContentCallback();
	if (err != SYNC_ERROR_NONE) {
		LOG_LOGD("DeRegistration of Media Content DataChangeListener Failed");
		return SYNC_ERROR_INVALID_OPERATION;
	}

	LOG_LOGD("DeRegistration of DataChangeListener Successfully Ends");

	return SYNC_ERROR_NONE;
}


void
DataChangeSyncScheduler::HandleDataChangeEvent(const char* pSyncCapability)
{
	LOG_LOGD("DataChangeSyncScheduler::HandleDataChangeEvent() Starts");

	pair<std::multimap<string, DataSyncJob*>::iterator, std::multimap<string, DataSyncJob*>::iterator> ret;
	ret = __dataChangeSyncJobs.equal_range(pSyncCapability);

	for (std::multimap<string, DataSyncJob*>::iterator it = ret.first; it != ret.second; ++it)
	{
		DataSyncJob* pDataSyncJob = it->second;
		SyncManager::GetInstance()->ScheduleSyncJob(pDataSyncJob, false);
	}

	SyncManager::GetInstance()->AlertForChange();

	LOG_LOGD("DataChangeSyncScheduler::HandleDataChangeEvent() Ends");
}


/* capability can be called as syncJobName in SyncManager  */
int
DataChangeSyncScheduler::AddDataSyncJob(string capability, DataSyncJob* dataSyncJob)
{
	__dataChangeSyncJobs.insert(make_pair(capability, dataSyncJob));

	return SYNC_ERROR_NONE;
}


void
DataChangeSyncScheduler::RemoveDataSyncJob(DataSyncJob* dataSyncJob)
{
	typedef multimap<string, DataSyncJob*>::iterator iterator;
	std::pair<iterator, iterator> iterpair = __dataChangeSyncJobs.equal_range(dataSyncJob->__capability);

	iterator it = iterpair.first;
	for (; it != iterpair.second; ++it)
	{
		if (it->second == dataSyncJob)
		{
			__dataChangeSyncJobs.erase(it);
		}
	}
}
