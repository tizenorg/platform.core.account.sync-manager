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


#ifndef __SYNC_MANAGER_H__
#define __SYNC_MANAGER_H__


#include <account.h>
#include <bundle.h>


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @file        sync_manager.h
 * @brief       This file contains Sync Manager APIs for application's data sync operations.
 */


/**
 * @addtogroup CAPI_SYNC_MANAGER_MODULE
 * @{
 */


/**
 *  @brief   Sync option to denote if retrying the sync request is allowed or not.
 *  @since_tizen 2.4
 */
#define SYNC_OPTION_NO_RETRY    "no_retry"


/**
 *  @brief   Sync option to denote if the sync request is to be handled on priority.
 *  @since_tizen 2.4
 */
#define SYNC_OPTION_EXPEDITED   "sync_expedited"


/**
 *  @brief   Enumerations for time intervals of a periodic sync.
 *  @since_tizen 2.4
 */
typedef enum
{
	SYNC_PERIOD_INTERVAL_5MIN = 0,		/**< Sync within 5 minutes */
	SYNC_PERIOD_INTERVAL_10MIN,			/**< Sync within 10 minutes */
	SYNC_PERIOD_INTERVAL_15MIN,			/**< Sync within 15 minutes */
	SYNC_PERIOD_INTERVAL_20MIN,			/**< Sync within 20 minutes */
	SYNC_PERIOD_INTERVAL_30MIN,			/**< Sync within 30 minutes */
	SYNC_PERIOD_INTERVAL_45MIN,			/**< Sync within 45 minutes */
	SYNC_PERIOD_INTERVAL_1H,			/**< Sync within 1 hour */
	SYNC_PERIOD_INTERVAL_2H,			/**< Sync within 2 hours */
	SYNC_PERIOD_INTERVAL_3H,			/**< Sync within 3 hours */
	SYNC_PERIOD_INTERVAL_6H,			/**< Sync within 6 hours */
	SYNC_PERIOD_INTERVAL_12H,			/**< Sync within 12 hours */
	SYNC_PERIOD_INTERVAL_1DAY,			/**< Sync within 1 day */
	SYNC_PERIOD_INTERVAL_MAX = SYNC_PERIOD_INTERVAL_1DAY + 1
} sync_period_e;


/**
 * @brief Connects to Sync Manager service.
 *
 * @since_tizen 2.4
 * @remarks Release this instance using sync_manager_disconnect().
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE             Successful
 * @retval #SYNC_ERROR_IO_ERROR         I/O error
 * @retval #SYNC_ERROR_SYSTEM           Internal system error
 * @retval #SYNC_ERROR_OUT_OF_MEMORY    Out of memory
 *
 * @see sync_manager_disconnect()
 */
int sync_manager_connect(void);


/**
 * @brief Disconnects from Sync Manager service and releases the resources it holds.
 *
 * @since_tizen 2.4
 *
 * @see sync_manager_connect()
 */
void sync_manager_disconnect(void);


/**
 * @brief Requests Sync Manager to perform one time sync operation.
 *
 * @since_tizen 2.4
 *
 * @param[in]   account        A valid account handle or @c NULL to request account less sync operation
 * @param[in]   capability     A string representing Provider ID of data control or capability of account or @c NULL in case of account less sync operation
 * @param[in]   extra          A bundle instance that contains sync related information
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE                 Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER    Invalid parameter
 * @retval #SYNC_ERROR_UNKNOWN              Unknown error
 * @retval #SYNC_ERROR_IO_ERROR             I/O error
 *
 * @pre This function requires connection to Sync Manager's instance.
 * @pre Call sync_manager_connect() before calling this function.
 *
 * @see sync_manager_connect()
 */
int sync_manager_add_sync_job(account_h account, const char* capability, bundle* extra);


/**
 * @brief Requests Sync Manager to remove corresponding sync request.
 *
 * @since_tizen 2.4
 *
 * @param[in]   account       A valid account handle or @c NULL in case of account less sync operation
 * @param[in]   capability    A string representing Provider ID of data control or capability of account or @c NULL in case of account less sync operation
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE                Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval #SYNC_ERROR_UNKNOWN             Unknown error
 * @retval #SYNC_ERROR_IO_ERROR            I/O error
 *
 * @pre This function requires connection to Sync Manager's instance.
 * @pre Call sync_manager_connect() before calling this function.
 *
 * @see sync_manager_connect()
 * @see sync_manager_add_sync_job()
 */
int sync_manager_remove_sync_job(account_h account, const char* capability);


/**
 * @brief Requests Sync Manager to perform periodic sync operations.
 *
 * @since_tizen 2.4
 *
 * @remarks Sync job can be added by capability. In the case of adding sync job with same capability, it will replace previous setting with new one.
 *
 * @param[in]   account         A valid account handle @c NULL to request account less sync operation
 * @param[in]   capability      A string representing Provider ID of data control or capability of account or @c NULL in case of account less sync operation
 * @param[in]   extra           A bundle instance that contains sync related information
 * @param[in]   sync_period     Determines time interval of periodic sync. However the actual sync period can be slightly delayed or ahead of this input.
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE                  Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER     Invalid parameter
 * @retval #SYNC_ERROR_UNKNOWN               Unknown error
 * @retval #SYNC_ERROR_IO_ERROR              I/O error
 *
 * @pre This function requires connection to Sync Manager's instance.
 * @pre Call sync_manager_connect() before calling this function.
 *
 * @see sync_manager_connect()
 * @see sync_period_e
 */
int sync_manager_add_periodic_sync_job(account_h account, const char* capability, bundle *extra, sync_period_e sync_period);


/**
 * @brief Requests Sync Manager to remove corresponding sync request.
 *
 * @since_tizen 2.4
 *
 * @param[in] account      A valid account handle @c NULL in case of account less sync operation
 * @param[in] capability   A string representing Provider ID of data control or capability of account @c NULL in case of account less sync operation
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE                Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER   Invalid parameter
 * @retval #SYNC_ERROR_UNKNOWN             Unknown error
 * @retval #SYNC_ERROR_IO_ERROR            I/O error
 *
 * @pre This function requires connection to Sync Manager's instance.
 * @pre Call sync_manager_connect() before calling this function.
 *
 * @see sync_manager_connect()
 * @see sync_manager_add_periodic_sync_job()
 */
int sync_manager_remove_periodic_sync_job(account_h account, const char* capability);


/* End of Sync Manager APIs */
/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* __SYNC_MANAGER_H__ */
