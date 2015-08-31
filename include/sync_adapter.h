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
 * @since_tizen 2.4
 *
 * @param[in] account          An account handle on which sync operation was requested or @c NULL in case of account less sync operation
 * @param[in] extra            Extras data which contains additional sync information
 * @param[in] capability       A string representing Provider ID of data control or capability of account or @c NULL in case of account less sync operation
 *
 * @return @c true if sync operation is success, @c false otherwise
 *
 * @pre The callback must be set using sync_adapter_set_callbacks().
 * @pre sync_manager_add_sync_job() or sync_manager_add_periodic_sync_job() calls this callback.
 *
 * @see sync_adapter_init()
 * @see sync_adapter_set_callbacks()
 * @see sync_manager_add_sync_job()
 * @see sync_manager_add_periodic_sync_job()
 */
typedef bool (*sync_adapter_start_sync_cb)(account_h account, bundle* extras, const char* capability);


/**
 * @brief Callback function for Sync Adapter's cancel sync request.
 *
 * @since_tizen 2.4
 *
 * @param[in] account      An account handle on which sync operation was requested or @c NULL in case of account less sync operation
 * @param[in] capability   A string representing Provider ID of data control or capability of account or @c NULL in case of account less sync operation
 *
 * @pre The callback must be set using sync_adapter_set_callbacks().
 * @pre sync_manager_remove_sync_job() or sync_manager_remove_periodic_sync_job() calls this callback.
 *
 * @see sync_adapter_init()
 * @see sync_adapter_set_callbacks()
 * @see sync_manager_remove_sync_job()
 * @see sync_manager_remove_periodic_sync_job()
 */
typedef void (*sync_adapter_cancel_sync_cb)(account_h account, const char* capability);


/**
 * @brief Initializes Sync Adapter's instance.
 *
 * @since_tizen 2.4
 * @remarks Destroy this instance using sync_adapter_destroy().
 *
 * @param[in] capability             Capability handled by this Sync Adapter application or @c NULL in case of account less sync operation
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE successful
 * @retval #SYNC_ERROR_OUT_OF_MEMORY Out of memory
 *
 * @see sync_adapter_destroy()
 */
int sync_adapter_init(const char* capability);


/**
 * @brief Destroys Sync Adapter's instance.
 *
 * @since_tizen 2.4
 *
 * @see sync_adapter_init()
 */
void sync_adapter_destroy(void);


/**
 * @brief Sets client (Sync Aadapter) callback functions
 *
 * @since_tizen 2.4
 *
 * @param[in] on_start_cb       A callback function to be called by Sync Manager for performing sync operation
 * @param[in] on_cancel_cb      A callback function to be called by Sync Manager for cancelling sync operation
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #SYNC_ERROR_NONE                Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval #SYNC_ERROR_UNKNOWN             Unknown error
 * @retval #SYNC_ERROR_SYSTEM              System error
 *
 * @pre This function requires intialized Sync Adapter's instance.
 * @pre Call sync_adapter_init() before calling this function.
 *
 * @see sync_adapter_init()
 * @see sync_adapter_start_sync_cb()
 * @see sync_adapter_cancel_sync_cb()
 */
int sync_adapter_set_callbacks(sync_adapter_start_sync_cb on_start_cb, sync_adapter_cancel_sync_cb on_cancel_cb);


/**
 * @brief Unsets client (Sync Adapter) callback functions
 *
 * @since_tizen 2.4
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 *
 * @retval #SYNC_ERROR_NONE                Successful
 * @retval #SYNC_ERROR_SYSTEM              System error
 *
 * @pre This function requires intialized Sync Adapter's instance.
 * @pre Call sync_adapter_set_callbacks() before calling this function.
 *
 * @see sync_adapter_init()
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


#endif // __SYNC_ADAPTER_H__
