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
 * @file	SyncManager_SyncWorker.cpp
 * @brief	This is the implementation file for the SyncWorker class.
 */

#include <sys/eventfd.h>
#include "sync-log.h"
#include "sync-error.h"
#include "SyncManager_SyncWorker.h"


/*namespace _SyncManager
{*/

SyncWorker::SyncWorker(void)
			: __pendingRequestsMutex(PTHREAD_MUTEX_INITIALIZER)
			, __message(SYNC_CHECK_ALARM)
			, __pContext(NULL)
			, __pLoop(NULL)
			, __pChannel(NULL)
			, __pSource(NULL)
			, __pThread(NULL)
{
}


SyncWorker::~SyncWorker(void)
{
	Finalize();
}


int
SyncWorker::FireEvent(ISyncWorkerResultListener* pSyncWorkerResultListener, Message msg)
{
	return AddRequestN(pSyncWorkerResultListener, msg);
}


void
SyncWorker::Initialize(void)
{
	LOG_LOGD("Initializing sync worker thread");

	GError* pGError = NULL;
	int ret;

	ret = pthread_mutex_init(&__pendingRequestsMutex, NULL);
	if (ret != 0)
	{
		LOG_ERRORD("__pendingRequestsMutex Initialise Failed %d", ret);
		return;
	}
	else
	{
		__pContext = g_main_context_new();
		ASSERT(__pContext);

		__pLoop = g_main_loop_new(__pContext, FALSE);
		ASSERT(__pLoop);

		int eventFd = eventfd(0, 0);
		__pChannel = g_io_channel_unix_new(eventFd);
		ASSERT(__pChannel);

		g_io_channel_set_encoding(__pChannel, NULL, &pGError);
		g_io_channel_set_flags(__pChannel, G_IO_FLAG_NONBLOCK, &pGError);
		g_io_channel_set_close_on_unref(__pChannel, TRUE);

		__pSource = g_io_create_watch(__pChannel, G_IO_IN);
		ASSERT(__pSource);

		g_source_set_callback(__pSource, (GSourceFunc) SyncWorker::OnEventReceived, this, NULL);
		g_source_attach(__pSource, __pContext);

		__pThread = g_thread_new("sync-thread", SyncWorker::ThreadLoop, static_cast<gpointer>(__pLoop));
		ASSERT(__pThread);
	}

	LOG_LOGD("SyncManager::initialize() Ends");
}

void
SyncWorker::Finalize(void)
{
	LOG_LOGD("Finalizing sync worker");

	for (std::list<RequestData*>::iterator it = __pendingRequests.begin(); it != __pendingRequests.end();)
	{
		RequestData* pRequestData = *it;
		delete pRequestData;
		pRequestData = NULL;
		it = __pendingRequests.erase(it);
	}
	pthread_mutex_destroy(&__pendingRequestsMutex);

	if (__pSource)
	{
		g_source_unref(__pSource);
		g_source_destroy(__pSource);
	}
	if (__pChannel)
	{
		g_io_channel_unref(__pChannel);
		g_io_channel_shutdown(__pChannel, TRUE, NULL);
	}
	if (__pLoop)
	{
		g_main_loop_unref(__pLoop);
		g_main_loop_quit(__pLoop);
	}
	if (__pContext)
	{
		g_main_context_unref(__pContext);
	}
	if (__pThread)
	{
		g_thread_exit(NULL);
	}
}


int
SyncWorker::AddRequestN(ISyncWorkerResultListener* pSyncWorkerResultListener, Message msg)
{
	SYNC_LOGE_RET_RES(__pChannel != NULL, SYNC_ERROR_SYSTEM, "IO channel is not setup");

	LOG_LOGD("Adding a request to sync worker");

	RequestData* pRequestData = new (std::nothrow) RequestData();
	if (pRequestData != NULL)
	{
		pRequestData->message = msg;
		pRequestData->pResultListener = pSyncWorkerResultListener;

		uint64_t count = 1;
		gsize writtenSize = 0;
		//		GError* error = NULL;
		//		GIOStatus status;

		pthread_mutex_lock(&__pendingRequestsMutex);
		__pendingRequests.push_back(pRequestData);
		LOG_LOGD("Added into __pendingRequests, current size = %d", __pendingRequests.size());
		pthread_mutex_unlock(&__pendingRequestsMutex);

		GError* pError = NULL;
		int status = g_io_channel_write_chars(__pChannel, (const gchar*) &count, sizeof(count), &writtenSize, &pError);
		if (status != G_IO_STATUS_NORMAL)
		{
			LOG_LOGD("SyncWorker::Add Request Failed with IO Write Error %d %d", status, count);
			return SYNC_ERROR_SYSTEM;
		}
		g_io_channel_flush (__pChannel, &pError);

		LOG_LOGD("Request Successfully added");
		return SYNC_ERROR_NONE;
	}

	return SYNC_ERROR_SYSTEM;
}


gboolean
SyncWorker::OnEventReceived(GIOChannel* pChannel, GIOCondition condition, gpointer data)
{
	LOG_LOGD("GIO event received");
	SyncWorker* pSyncWorker = static_cast<SyncWorker*>(data);
	SYNC_LOGE_RET_RES(pSyncWorker != NULL, FALSE, "Data is NULL");

	if ((condition & G_IO_IN) != 0) 
	{
		uint64_t tmp = 0;
		gsize readSize = 0;
		GError* pError = NULL;
		GIOStatus status;

		status = g_io_channel_read_chars (pChannel, (gchar*)&tmp, sizeof(tmp), &readSize, &pError);

		if (readSize == 0 || status != G_IO_STATUS_NORMAL)
		{
			LOG_LOGD("Failed with IO Read Error");
			return TRUE;
		}

		pthread_mutex_lock(&pSyncWorker->__pendingRequestsMutex);
		std::list<RequestData*>::iterator it;
		it = pSyncWorker->__pendingRequests.begin();
		if (*it != NULL)
		{
			RequestData* pData = *it;
			pSyncWorker->__pendingRequests.pop_front();
			LOG_LOGD("Popping from __pendingRequests, size = %d", pSyncWorker->__pendingRequests.size());
			pthread_mutex_unlock(&pSyncWorker->__pendingRequestsMutex);

			pData->pResultListener->OnEventReceived(pData->message);

			delete pData;
			pData = NULL;
		}
		else
		{
			pthread_mutex_unlock(&pSyncWorker->__pendingRequestsMutex);
		}

		LOG_LOGD("Event handled successfully");
		return TRUE;
	}

	LOG_LOGD("UnSuccessfully Ends");
	return FALSE;
}


gpointer
SyncWorker::ThreadLoop(gpointer data)
{
	LOG_LOGD("Sync worker thread  entered");

	GMainLoop* pLoop = static_cast<GMainLoop*>(data);
	if (pLoop != NULL)
	{
		LOG_LOGD("Thread loop Running");
		g_main_loop_run(pLoop);
	}

	LOG_LOGD("Sync worker thread ends");

	return NULL;
}
//}//_SyncManager
