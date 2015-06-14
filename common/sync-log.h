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


#ifndef SYNC_LOG_H_
#define SYNC_LOG_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SYNCSERVICE"
#define LOG_TAG_CLIENT "SYNCCLIENT"

#include <dlog.h>
#include <stdbool.h>

#define TIZEN_DEBUG_ENABLE
#define ENABLE_DEBUG

#define E_SUCCESS true
#define E_FAILURE false
typedef bool  result;

#ifdef ENABLE_DEBUG

#define LOG_VERBOSE(module, fmt, arg...) LOGI(fmt "\n", ##arg)
#define LOG_ERROR(module, fmt, arg...) LOGE(" * Critical: " fmt "\n", ##arg)

#define LOG_LOGD(x...)	LOG_VERBOSE(LOG_TAG, ##x)
#define LOG_LOGC(x...)	LOG_VERBOSE(LOG_TAG, ##x)
#define LOG_ERRORD(x...)	LOG_ERROR(LOG_TAG, ##x)

#define LOG_LOGE_RESULT(condition, x...) \
		if (!(condition)) { \
			LOG_ERROR(LOG_TAG, ##x); \
			return E_FAILURE; \
		} \
		else { ;}

#define SYNC_LOGE_RET_RES(condition, retX,x...) \
		if (!(condition)) { \
			LOG_VERBOSE(LOG_TAG, ##x); \
			return retX; \
		} \
		else { ;}

#define LOG_LOGE_NULL(condition, x...) \
		if (!(condition)) { \
			LOG_ERROR(LOG_TAG, ##x); \
			return NULL; \
		} \
		else { ;}

#define LOG_LOGE_VOID(condition, x...) \
		if (!(condition)) { \
			LOG_ERROR(LOG_TAG, ##x); \
			return ; \
		} \
		else { ;}

#define LOG_LOGE_BOOL(condition, x...) \
		if (!(condition)) { \
			LOG_ERROR(LOG_TAG, ##x); \
			return false; \
		} \
		else { ;}

#define SYNC_LOG_IF(condition, x...) \
	if ((condition)) { \
		LOG_ERROR(LOG_TAG, ##x); \
	} \

#define ASSERT(expr) \
		if (!(expr)) \
		{   \
			LOG_ERRORD("Assertion %s", #expr); \
			return;\
		} \


#else

#define LOG_LOGD(x...)
#define LOG_LOGE(x...)
#endif // ENABLE_DEBUG_MSG



#endif /* SYNC_LOG_H_ */
