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
 *  @brief		This is contact capability string.
 *  @since_tizen 3.0
 *  @remarks	If you want to receive notification about contact DB change, add it through sync_manager_add_data_change_sync_job().
 *  @see		sync_manager_add_data_change_sync_job()
  */
#define SYNC_SUPPORTS_CAPABILITY_CONTACT	"http://tizen.org/sync/capability/contact"


/**
 *  @brief		This is image capability string.
 *  @since_tizen 3.0
 *  @remarks	If you want to receive notification about media image DB change, add it through sync_manager_add_data_change_sync_job().
 *  @see		sync_manager_add_data_change_sync_job()
 */
#define SYNC_SUPPORTS_CAPABILITY_IMAGE	"http://tizen.org/sync/capability/image"


/**
 *  @brief		This is video capability string.
 *  @since_tizen 3.0
 *  @remarks	If you want to receive notification about media video DB change, add it through sync_manager_add_data_change_sync_job().
 *  @see		sync_manager_add_data_change_sync_job()
 */
#define SYNC_SUPPORTS_CAPABILITY_VIDEO	"http://tizen.org/sync/capability/video"


/**
 *  @brief		This is sound capability string.
 *  @since_tizen 3.0
 *  @remarks	If you want to receive notification about media sound DB change, add it through sync_manager_add_data_change_sync_job().
 *  @see		sync_manager_add_data_change_sync_job()
 */
#define SYNC_SUPPORTS_CAPABILITY_SOUND	"http://tizen.org/sync/capability/sound"


/**
 *  @brief		This is music capability string.
 *  @since_tizen 3.0
 *  @remarks	If you want to receive notification about media music DB change, add it through sync_manager_add_data_change_sync_job().
 *  @see		sync_manager_add_data_change_sync_job()
 */
#define SYNC_SUPPORTS_CAPABILITY_MUSIC	"http://tizen.org/sync/capability/music"


/**
 *  @brief   Enumerations for sync options of sync job request APIs.
 *  @since_tizen 3.0
 */
typedef enum {
	SYNC_OPTION_NONE = 0,														/**< Sync job will be operated normally */
	SYNC_OPTION_EXPEDITED = 0x01,												/**< Sync job will be operated as soon as possible */
	SYNC_OPTION_NO_RETRY = 0x02,												/**< Sync job will not be performed again when it fails */
} sync_option_e;


/**
 *  @brief   Enumerations for time intervals of a periodic sync.
 *  @since_tizen 3.0
 */
typedef enum {
	SYNC_PERIOD_INTERVAL_30MIN = 0,		/**< Sync within 30 minutes */
	SYNC_PERIOD_INTERVAL_1H,			/**< Sync within 1 hour */
	SYNC_PERIOD_INTERVAL_2H,			/**< Sync within 2 hours */
	SYNC_PERIOD_INTERVAL_3H,			/**< Sync within 3 hours */
	SYNC_PERIOD_INTERVAL_6H,			/**< Sync within 6 hours */
	SYNC_PERIOD_INTERVAL_12H,			/**< Sync within 12 hours */
	SYNC_PERIOD_INTERVAL_1DAY,			/**< Sync within 1 day */
	SYNC_PERIOD_INTERVAL_MAX = SYNC_PERIOD_INTERVAL_1DAY + 1
} sync_period_e;


/**
 * @brief Called to get the information once for each sync job.
 *
 * @since_tizen 3.0
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_job_name			A string representing a sync job which has been operated or @c NULL in the case of data change sync operation
 * @param[in] sync_capability		A string representing a sync job which has been operated because of data change or @c NULL in the case of on demand and periodic sync operation
 * @param[in] sync_job_id			A unique value which can manage sync jobs
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job or it can be @c NULL in the case of requesting without sync_job_user_data
 * @param[in] user_data				User data which contains additional information related foreach job or it can be @c NULL in the case of querying without user_data
 *
 * @return @c true to continue with the next iteration of the loop, otherwise @c false to break out of the loop
 *
 * @pre sync_manager_foreach_sync_job() calls this callback.
 *
 * @see sync_adapter_set_callbacks()
 * @see sync_manager_foreach_sync_job()
 */
typedef bool (*sync_manager_sync_job_cb)(account_h account, const char *sync_job_name, const char *sync_capability, int sync_job_id, bundle *sync_job_user_data, void *user_data);


/**
 * @brief Requests Sync Manager to perform one time sync operation.
 *
 * @since_tizen 3.0
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_job_name			A string representing a sync job which will be operated just one time
 * @param[in] sync_option			sync options determine an way to operate sync job and can be used as ORing.
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job or it can be @c NULL in the case of requesting without sync_job_user_data
 * @param[out] sync_job_id			A unique value which can manage sync jobs. The number of sync job id is limited as less than a hundred.
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE									Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER					Invalid parameter
 * @retval #SYNC_ERROR_QUOTA_EXCEEDED						Quota exceeded
 * @retval #SYNC_ERROR_SYSTEM								Internal system error
 * @retval #SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND				Sync adapter is not registered
 *
 * @pre This function requires calling below Sync Adapter's APIs by a service application before it is called.
 * @pre Call sync_adapter_set_callbacks() before calling this function.
 *
 * @see sync_manager_remove_sync_job()
 * @see sync_option_e
 */
int sync_manager_on_demand_sync_job(account_h account, const char *sync_job_name, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id);


/**
 * @brief Requests Sync Manager to perform periodic sync operations.
 *
 * @since_tizen 3.0
 *
 * @privlevel	public
 * @privilege	%http://tizen.org/privilege/alarm.set
 *
 * @remarks Sync job can be added with its name. In the case of adding periodic sync job with same sync job, it will replace previous setting with new one.
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_job_name			A string representing a sync job which will be operated with period interval
 * @param[in] sync_period			Determines time interval of periodic sync. The periodic sync operation can be triggered in that interval, but it does not guarantee exact time. The minimum value is 30 minutes.
 * @param[in] sync_option			sync options determine an way to operate sync job and can be used as ORing.
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job or it can be @c NULL in the case of requesting without sync_job_user_data
 * @param[out] sync_job_id			A unique value which can manage sync jobs. The number of sync job id is limited as less than a hundred.
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE								Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER				Invalid parameter
 * @retval #SYNC_ERROR_QUOTA_EXCEEDED					Quota exceeded
 * @retval #SYNC_ERROR_SYSTEM							Internal system error
 * @retval #SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND			Sync adapter is not registered
 *
 * @pre This function requires calling below Sync Adapter's APIs by a service application before it is called.
 * @pre Call sync_adapter_set_callbacks() before calling this function.
 *
 * @see sync_manager_remove_sync_job()
 * @see sync_option_e
 * @see sync_period_e
 */
int sync_manager_add_periodic_sync_job(account_h account, const char *sync_job_name, sync_period_e sync_period, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id);


/**
 * @brief Requests Sync Manager to perform sync operations whenever corresponding DB changed.
 *
 * @since_tizen 3.0
 * @privlevel	public
 * @privilege	%http://tizen.org/privilege/contact.read
 *
 * @remarks Data change sync job can be added by using its capability. In the case of adding a sync job with same capability, it will replace previous setting with new one. \n\n
 * %http://tizen.org/privilege/contact.read is needed to add data change sync job for receiving notification with @ref SYNC_SUPPORTS_CAPABILITY_CONTACT.
 *
 * @param[in] account				An account handle on which sync operation was requested or @c NULL in the case of accountless sync operation
 * @param[in] sync_capability		A string representing a sync job which will be operated whenever data change of this capability
 * @param[in] sync_option			sync options determine an way to operate sync job and can be used as ORing.
 * @param[in] sync_job_user_data	User data which contains additional information related registered sync job or it can be @c NULL in the case of requesting without sync_job_user_data
 * @param[out] sync_job_id			A unique value which can manage sync jobs. The number of sync job id is limited as less than a hundred.
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE							Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER			Invalid parameter
 * @retval #SYNC_ERROR_QUOTA_EXCEEDED				Quota exceeded
 * @retval #SYNC_ERROR_SYSTEM						Internal system error
 * @retval #SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND		Sync adapter is not registered
 *
 * @pre This function requires calling below Sync Adapter's APIs by a service application before it is called.
 * @pre Call sync_adapter_set_callbacks() before calling this function.
 *
 * @see sync_manager_remove_sync_job()
 * @see sync_option_e
 */
int sync_manager_add_data_change_sync_job(account_h account, const char *sync_capability, sync_option_e sync_option, bundle *sync_job_user_data, int *sync_job_id);


/**
 * @brief Requests Sync Manager to remove corresponding sync job id.
 *
 * @since_tizen 3.0
 *
 * @remarks			sync_job_id can not be @c NULL.
 *
 * @param[in] sync_job_id			A unique value of each sync job, it can be used to search specific sync job and remove it
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE					Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER	Invalid parameter
 * @retval #SYNC_ERROR_SYSTEM				Internal system error
 *
 * @pre This function requires calling at least one of the below Sync Manager's APIs before it is called.
 * @pre sync_manager_on_demand_sync_job()
 * @pre sync_manager_add_periodic_sync_job()
 * @pre sync_manager_add_data_change_sync_job()
 *
 * @see sync_manager_on_demand_sync_job()
 * @see sync_manager_add_periodic_sync_job()
 * @see sync_manager_add_data_change_sync_job()
 */
int sync_manager_remove_sync_job(int sync_job_id);


/**
 * @brief Requests Sync Manager to query corresponding sync request.
 *
 * @since_tizen 3.0
 *
 * @param[in] sync_job_cb			A callback function for receiving the result of this API
 * @param[in] user_data				User data which contains additional information related foreach job or @c NULL if do not want to transfer user data
 *
 * @return @c 0 on success,
 *         otherwise a negative error value
 * @retval #SYNC_ERROR_NONE					Successful
 * @retval #SYNC_ERROR_INVALID_PARAMETER	Invalid parameter
 * @retval #SYNC_ERROR_SYSTEM				Internal system error
 *
 * @pre This function requires calling at least one of the below Sync Manager's APIs before it is called.
 * @pre sync_manager_on_demand_sync_job()
 * @pre sync_manager_add_periodic_sync_job()
 * @pre sync_manager_add_data_change_sync_job()
 *
 * @see sync_manager_sync_job_cb()
 * @see sync_manager_on_demand_sync_job()
 * @see sync_manager_add_periodic_sync_job()
 * @see sync_manager_add_data_change_sync_job()
 */
int sync_manager_foreach_sync_job(sync_manager_sync_job_cb sync_job_cb, void *user_data);


/* End of Sync Manager APIs */
/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* __SYNC_MANAGER_H__ */
