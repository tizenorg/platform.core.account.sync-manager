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


#ifndef __SYNC_ERROR_H__
#define __SYNC_ERROR_H__


#include <tizen_error.h>


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * @file   sync-error.h
 * @brief  This file contains Sync Manager's error definitions.
 */


/**
 * @addtogroup CAPI_SYNC_MANAGER_MODULE
 * @{
 */


/**
 *  @brief    Enumerations of error codes for Sync Manager APIs.
 *  @since_tizen 2.4
 */
typedef enum {
	SYNC_ERROR_NONE						= TIZEN_ERROR_NONE,					/**< Successful */
	SYNC_ERROR_OUT_OF_MEMORY			= TIZEN_ERROR_OUT_OF_MEMORY,		/**< Out of memory */
	SYNC_ERROR_IO_ERROR					= TIZEN_ERROR_IO_ERROR,				/**< I/O error */
	SYNC_ERROR_PERMISSION_DENIED		= TIZEN_ERROR_PERMISSION_DENIED,	/**< Permission denied */
	SYNC_ERROR_ALREADY_IN_PROGRESS		= TIZEN_ERROR_ALREADY_IN_PROGRESS,	/**< Duplicate data */
	SYNC_ERROR_INVALID_OPERATION		= TIZEN_ERROR_INVALID_OPERATION,	/**< Error in Operation */
	SYNC_ERROR_INVALID_PARAMETER		= TIZEN_ERROR_INVALID_PARAMETER,	/**< Invalid parameter */
	SYNC_ERROR_QUOTA_EXCEEDED			= TIZEN_ERROR_QUOTA_EXCEEDED,		/**< Quota exceeded */
	SYNC_ERROR_UNKNOWN					= TIZEN_ERROR_UNKNOWN,				/**< Unknown Error */
	SYNC_ERROR_SYSTEM					= TIZEN_ERROR_SYNC_MANAGER | 0x01,	/**< System error */
	SYNC_ERROR_SYNC_ADAPTER_NOT_FOUND	= TIZEN_ERROR_SYNC_MANAGER | 0x02	/**< Sync adater is not registered */
} sync_error_e;


/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* __SYNC_ERROR_H__ */
