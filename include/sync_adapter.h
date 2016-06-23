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


#ifndef __SYNC_ADAPTER_H__
#define __SYNC_ADAPTER_H__


#include <account.h>
#include <bundle.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @file        sync_adapter.h
 * @brief       This file contains Sync Adapter APIs and the callbacks.
 */


/**
 * @addtogroup CAPI_SYNC_ADAPTER_MODULE
 * @{
 */


/**
 * @brief Callback function for Sync Adapter's start sync request.
 *
 * @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
 * @remarks	This API only can be called at a service application.\n\n
 * Release account with account_destroy() after using it.\n\n
 * Release sync_job_user_data with bundle_free() after using it.
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_job_name			A string representing a sync job which has been operated or @c NULL in the case of data change sync operation
 * @param[in] sync_capability		A string representing a sync job which has been operated because of data change or @c NULL in the case of on demand or periodic sync operation
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job
 *
 * @return @c true if sync operation is success, @c false otherwise
 *
 * @pre The callback must be set by using sync_adapter_set_callbacks().
 * @pre sync_manager_on_demand_sync_job() calls this callback.
 * @pre sync_manager_add_periodic_sync_job() calls this callback.
 * @pre sync_manager_add_data_change_sync_job() calls this callback.
 *
 * @see sync_adapter_set_callbacks()
 * @see sync_manager_on_demand_sync_job()
 * @see sync_manager_add_periodic_sync_job()
 * @see sync_manager_add_data_change_sync_job()
 */
typedef bool (*sync_adapter_start_sync_cb)(account_h account, const char *sync_job_name, const char *sync_capability, bundle *sync_job_user_data);


/**
 * @brief Callback function for Sync Adapter's cancel sync request.
 *
 * @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
 * @remarks	This API only can be called at a service application after calling sync_manager_remove_sync_job().\n\n
 * Release account with account_destroy() after using it.\n\n
 * Release sync_job_user_data with bundle_free() after using it.
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_job_name			A string representing a sync job which has been operated or @c NULL in the case of data change sync operation
 * @param[in] sync_capability		A string representing a sync job which has been operated because of data change or @c NULL in the case of on demand and periodic sync operation
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job
 *
 * @pre The callback must be set by using sync_adapter_set_callbacks().
 * @pre sync_manager_remove_sync_job() calls this callback in the case there is removable sync job.
 *
 * @see sync_adapter_set_callbacks()
 * @see sync_manager_remove_sync_job()
 */
typedef void (*sync_adapter_cancel_sync_cb)(account_h account, const char *sync_job_name, const char *sync_capability, bundle *sync_job_user_data);


/**
 * @brief Sets client (Sync Adapter) callback functions
 *
 * @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
 * @remarks	This API only can be called by a service application. And it can be set by only one service application per a package.
 *
 * @param[in] on_start_cb       A callback function to be called by Sync Manager for performing sync operation
 * @param[in] on_cancel_cb      A callback function to be called by Sync Manager for cancelling sync operation
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #SYNC_ERROR_NONE					Successful
 * @retval #SYNC_ERROR_OUT_OF_MEMORY		Out of memory
 * @retval #SYNC_ERROR_IO_ERROR				I/O error
 * @retval #SYNC_ERROR_INVALID_PARAMETER	Invalid parameter
 * @retval #SYNC_ERROR_QUOTA_EXCEEDED		Quota exceeded
 * @retval #SYNC_ERROR_SYSTEM				System error
 *
 * @see sync_adapter_start_sync_cb()
 * @see sync_adapter_cancel_sync_cb()
 * @see sync_adapter_unset_callbacks()
 */
int sync_adapter_set_callbacks(sync_adapter_start_sync_cb on_start_cb, sync_adapter_cancel_sync_cb on_cancel_cb);


/**
 * @brief Unsets client (Sync Adapter) callback functions
 *
 * @since_tizen @if MOBILE 2.4 @elseif WEARABLE 3.0 @endif
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #SYNC_ERROR_NONE                Successful
 * @retval #SYNC_ERROR_SYSTEM              System error
 *
 * @pre Call sync_adapter_set_callbacks() before calling this function.
 *
 * @see sync_adapter_start_sync_cb()
 * @see sync_adapter_cancel_sync_cb()
 * @see sync_adapter_set_callbacks()
 */
int sync_adapter_unset_callbacks(void);


/* End of Sync Adapter APIs */
/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* __SYNC_ADAPTER_H__ */
