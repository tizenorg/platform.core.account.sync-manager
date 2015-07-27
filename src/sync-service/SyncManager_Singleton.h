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

#ifndef SYNC_SERVICE_SINGLETON_H
#define SYNC_SERVICE_SINGLETON_H

#include <stdlib.h>
#include "sync-log.h"

/*namespace _SyncManager
{
*/

template<typename TYPE>
class Singleton
{
public:
	static TYPE* GetInstance(void)
	{
		if (__pInstance == NULL)
		{
			LOG_LOGD("singleton creation called");
			__pInstance = new (std::nothrow) TYPE;
			if (__pInstance == NULL)
			{
				LOG_LOGD("heap error");
			}
		}

		return __pInstance;
	}
	static void Destroy(void)
	{
	}

protected:
	Singleton(void) {}

	virtual ~Singleton(void){}

private:
	Singleton(const Singleton& obj);

	Singleton& operator=( const Singleton& obj);

private:
	static TYPE* __pInstance;
};

template<typename TYPE>
TYPE* Singleton<TYPE>::__pInstance = NULL;

//}//_SyncManager
#endif // SYNC_SERVICE_SINGLETON_H
