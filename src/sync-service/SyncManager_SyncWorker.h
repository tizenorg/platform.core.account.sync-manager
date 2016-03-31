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
 * @file	SyncManager_SyncWorker.h
 * @brief	This is the header file for the SyncWorker class.
 */

#ifndef SYNC_SERVICE_SYNC_WORKER_H
#define SYNC_SERVICE_SYNC_WORKER_H

#include <glib.h>
#include <pthread.h>
#include <list>
#include <stdlib.h>
#include "SyncManager_SyncWorkerResultListener.h"
#include "SyncManager_SyncDefines.h"


/*namespace _SyncManager
{
*/

using namespace std;


class SyncWorker
{
public:
	SyncWorker(void);

	virtual ~SyncWorker(void);

	int FireEvent(ISyncWorkerResultListener* pSyncWorkerResultListener, Message msg);

private:
	SyncWorker(const SyncWorker &obj);

	SyncWorker &operator = (const SyncWorker &obj);

	void Initialize(void);

	void Finalize(void);

	int AddRequestN(ISyncWorkerResultListener* pSyncWorkerResultListener, Message msg);

	static gboolean OnEventReceived(GIOChannel *pChannel, GIOCondition condition, gpointer data);

	static gpointer ThreadLoop(gpointer data);

private:
	struct RequestData {
		ISyncWorkerResultListener* pResultListener;
		Message message;
	};

std::list < RequestData * > __pendingRequests;

	pthread_mutex_t __pendingRequestsMutex;
	SyncDispatchMessage __message;
	GMainContext* __pContext;
	GMainLoop* __pLoop;
	GIOChannel* __pChannel;
	GSource* __pSource;
	GThread* __pThread;

	friend class SyncManager;
};

/*
} _SyncManager
*/

#endif /* SYNC_SERVICE_SYNC_WORKER_H */
