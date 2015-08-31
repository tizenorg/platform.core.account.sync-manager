# Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT("sync-service")

IF(NOT DEFINED PACKAGE_NAME)
	SET(PACKAGE_NAME "sync-service")
ENDIF(NOT DEFINED PACKAGE_NAME)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
IF(NOT DEFINED BINDIR)
	SET(BINDIR"${PREFIX}/bin")
ENDIF(NOT DEFINED BINDIR)

#IF(NOT DEFINED SYSTEMD_DIR)
#	SET(SYSTEMD_DIR "${LIBDIR}/systemd/system")
#ENDIF(NOT DEFINED SYSTEMD_DIR)

IF(NOT DEFINED INCLUDEDIR)
	SET(INCLUDEDIR "${PREFIX}/include")
ENDIF(NOT DEFINED INCLUDEDIR)


INCLUDE_DIRECTORIES(
	./
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/common
	)

SET(SRCS
	main.cpp
	SyncManager_ServiceInterface.cpp
	SyncManager_CapabilityInfo.cpp
	SyncManager_PeriodicSyncJob.cpp
	SyncManager_SyncManager.cpp
	SyncManager_SyncService.cpp
	SyncManager_NetworkChangeListener.cpp
	SyncManager_StorageChangeListener.cpp
	SyncManager_CurrentSyncJobQueue.cpp
	SyncManager_RepositoryEngine.cpp
	SyncManager_BatteryStatusListener.cpp
	SyncManager_SyncJob.cpp
	SyncManager_SyncJobQueue.cpp
	SyncManager_SyncStatusInfo.cpp
	SyncManager_SyncDefines.cpp
	SyncManager_SyncJobDispatcher.cpp
	${CMAKE_SOURCE_DIR}/common/sync-ipc-marshal.c
	SyncManager_SyncWorker.cpp
	SyncManager_CurrentSyncContext.cpp
	SyncManager_DataChangeSyncScheduler.cpp
	SyncManager_PeriodicSyncScheduler.cpp
	SyncManager_SyncAdapterAggregator.cpp
	SyncManager_SyncJobsAggregator.cpp
	SyncManager_SyncJobsInfo.cpp
	SyncManager_DataSyncJob.cpp)


#IF(_SEC_FEATURE_CONTAINER_ENABLE)
#	ADD_DEFINITIONS("-D_SEC_FEATURE_CONTAINER_ENABLE=1")
#ENDIF(_SEC_FEATURE_CONTAINER_ENABLE)

ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE_NAME}\"")

INCLUDE(FindPkgConfig)
SET(PACKAGES
		ecore
		capi-network-connection
		capi-system-runtime-info
		capi-system-device
		capi-appfw-application
		capi-appfw-app-manager
		pkgmgr
		pkgmgr-info
		dlog
		db-util
		appsvc
		appcore-efl
		glib-2.0
		capi-system-info
		vconf
		vconf-internal-keys
		accounts-svc
		libxml-2.0
		gio-unix-2.0
		alarm-service
		bundle
		calendar-service2
		contacts-service2
		libtzplatform-config
		capi-content-media-content)

IF(_SEC_FEATURE_CONTAINER_ENABLE)
	SET(PACKAGES ${PACKAGES} vasum)
ENDIF(_SEC_FEATURE_CONTAINER_ENABLE)

pkg_check_modules(PKGS REQUIRED ${PACKAGES})

FOREACH(flag ${PKGS_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fvisibility=hidden -fPIE")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
SET(PKGS_LDFLAGS "${PKGS_LDFLAGS} -pie")

ADD_CUSTOM_COMMAND(
	WORKING_DIRECTORY
	OUTPUT sync-adapter-stub.c
	COMMAND gdbus-codegen --interface-prefix org.
	--generate-c-code sync-adapter-stub ../../sync_adapter.xml
	COMMENT "Generating Sync adapter GDBus .c/.h")

ADD_CUSTOM_COMMAND(
	WORKING_DIRECTORY
	OUTPUT sync-manager-stub.c
	COMMAND gdbus-codegen --interface-prefix org.
	--generate-c-code sync-manager-stub ../../sync_manager.xml
	COMMENT "Generating Sync Service GDBus .c/.h")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS} sync-manager-stub.c sync-adapter-stub.c)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS} "-ldl")
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} core-sync-client)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
#INSTALL(FILES ${CMAKE_SOURCE_DIR}/packaging/sync-manager.service DESTINATION ${SYSTEMD_DIR})
