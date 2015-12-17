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
 * @file	SyncManager_SyncStatusInfo.cpp
 * @brief	This is the implementation file for the SyncStatusInfo class.
 */


#include <sstream>
#include "sync-log.h"
#include "SyncManager_SyncStatusInfo.h"


/*namespace _SyncManager
{*/


SyncStatusInfo::SyncStatusInfo(int capabilityId)
{
	this->capabilityId = capabilityId;
}


SyncStatusInfo::SyncStatusInfo(SyncStatusInfo &other)
{
	this->capabilityId = other.capabilityId;

	vector<long long> ::iterator it;
	for (it = other.__periodicSyncTimes.begin(); it != other.__periodicSyncTimes.end(); it++) {
		long long val = (*it);
		this->__periodicSyncTimes.push_back(val);
	}
}


SyncStatusInfo&
SyncStatusInfo::operator =(SyncStatusInfo& other)
{
	this->capabilityId = other.capabilityId;

	vector<long long> ::iterator it;
	for (it = other.__periodicSyncTimes.begin(); it != other.__periodicSyncTimes.end(); it++) {
		long long val = (*it);
		this->__periodicSyncTimes.push_back(val);
	}
	return *this;
}


SyncStatusInfo::SyncStatusInfo(string statusInfo)
				: capabilityId(-1)
{
	if (statusInfo.empty()) {
		LOG_LOGD("statusInfo string empty");
		return;
	}
	stringstream ss(statusInfo);

	int capabilityId = 0;
	int periodicSyncSize = 0;

	ss >> capabilityId;
	if (ss) {
		ss >> periodicSyncSize;
        if (periodicSyncSize <= 0) {
            LOG_LOGD("statusInfo corrupted");
            return;
        }
	}

	this->capabilityId = capabilityId;

	for (int i = 0; i < periodicSyncSize; i++) {
		int periodicSyncTime;
		ss >> periodicSyncTime;
		this->__periodicSyncTimes.push_back(periodicSyncTime);
	}
}


void
SyncStatusInfo::SetPeriodicSyncTime(unsigned int index, long long when)
{
	// The list is initialized lazily when scheduling occurs so we need to make sure
	// we initialize elements < index to zero (zero is ignore for scheduling purposes)
	EnsurePeriodicSyncTimeSize(index);
	__periodicSyncTimes[index] = when;
}


long long
SyncStatusInfo::GetPeriodicSyncTime(unsigned int index)
{
	if (index < __periodicSyncTimes.size()) {
		return __periodicSyncTimes[index];
	}
	else {
		return 0;
	}
}


void
SyncStatusInfo::RemovePeriodicSyncTime(unsigned int index)
{
	if (index < __periodicSyncTimes.size()) {
		__periodicSyncTimes.erase(__periodicSyncTimes.begin()+index);
	}
}


void
SyncStatusInfo::EnsurePeriodicSyncTimeSize(unsigned int index)
{
	unsigned int requiredSize = index + 1;
	if (__periodicSyncTimes.size() < requiredSize) {
		for (unsigned int i = __periodicSyncTimes.size(); i < requiredSize; i++) {
			__periodicSyncTimes.push_back((long) 0);
		}
	}
}


string
SyncStatusInfo::GetStatusInfoString(void)
{
	stringstream ss;
	string buff;

	if (__periodicSyncTimes.size() > 0) {
		ss << capabilityId;
		buff.append(ss.str().c_str());
		ss.str(string());

		LOG_LOGD("writing sync time now, size = %d", __periodicSyncTimes.size());
		buff.append(" ");
		ss << __periodicSyncTimes.size();
		buff.append(ss.str().c_str());
		ss.str(string());

		for (unsigned int i = 0; i < __periodicSyncTimes.size(); i++) {
			LOG_LOGD("writing sync time %lld", __periodicSyncTimes[i]);
			buff.append(" ");
			ss << __periodicSyncTimes[i];
			buff.append(ss.str().c_str());
			ss.str(string());
		}
	}

	return buff;
}
//}//_SyncManager
