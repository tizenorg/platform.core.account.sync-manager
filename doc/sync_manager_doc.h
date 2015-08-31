/*
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __SYNC_MANAGER_DOC_H__
#define __SYNC_MANAGER_DOC_H__

/**
 * @defgroup  CAPI_SYNC_MANAGER_MODULE Sync Manager
 * @ingroup   CAPI_ACCOUNT_FRAMEWORK
 * @brief     The Sync Manager API helps applications in scheduling their data sync operations.
 *
 * @section   CAPI_SYNC_MANAGER_HEADER Required Header
 *  \#include <sync_manager.h>
 *
 * @section CAPI_SYNC_MANAGER_MODULE_OVERVIEW Overview
 * The Sync Manager provides APIs for managing the sync operations. Applications will call these APIs to schedule their sync operations.
 * Sync Manager maintains sync requests from all the applications and invokes their respective callback methods to perform data synchronization operations.
 *
 * @defgroup  CAPI_SYNC_ADAPTER_MODULE Sync Adapter
 * @ingroup   CAPI_SYNC_MANAGER_MODULE
 * @brief     The Sync Adapter.
 *
 * @section   CAPI_SYNC_ADAPTER_HEADER Required Header
 *  \#include <sync_adapter.h>
 *
 * @section CAPI_SYNC_ADAPTER_MODULE_OVERVIEW Overview
 * Sync Adapter contains the callbacks required sync manager for triggering sync operations. Applications have to implement this sync adapter callback functions.
 * The callback functions holds the logic of data sync operations.
 * Each service application operates sync job respectively through the app control mechanism. The app control is operated by using appId. Also, it can distinguish start or cancel sync operation through app control operations which are written in Sync programming guide.
*/

#endif /* __SYNC_MANAGER_DOC_H__  */
