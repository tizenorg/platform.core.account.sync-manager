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
PROJECT("core-sync-client")

SET(this_target "core-sync-client")

IF(NOT DEFINED PACKAGE_NAME)
	SET(PACKAGE_NAME "core-sync-client")
ENDIF(NOT DEFINED PACKAGE_NAME)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
#SET(LIBDIR "${PREFIX}/lib")
#SET(INCLUDEDIR "${PREFIX}/include")
SET(VERSION_MAJOR 1)
SET(FULLVER "${VERSION_MAJOR}.0")

IF(_FEATURE_CALENDAR_ENABLE)
	ADD_DEFINITIONS("-D_FEATURE_CALENDAR_ENABLE=1")
ENDIF(_FEATURE_CALENDAR_ENABLE)


INCLUDE_DIRECTORIES(
#		./
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/common
)

IF(_FEATURE_MOBILE_PROFILE)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/mobile)
ELSE(_FEATURE_MOBILE_PROFILE)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include/wearable)
ENDIF(_FEATURE_MOBILE_PROFILE)

SET(SRCS sync_manager.c sync_adapter.c ${CMAKE_SOURCE_DIR}/common/sync-ipc-marshal.c
)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/cmake_build_tmp/output)

ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE_NAME}\"")

INCLUDE(FindPkgConfig)
pkg_check_modules(PKGS REQUIRED
		capi-appfw-app-manager
		dlog
		gio-unix-2.0
		bundle)

FOREACH(flag ${PKGS_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

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
	COMMENT "Generating Sync Client GDBus .c/.h")

## Create Library
ADD_LIBRARY(${this_target} SHARED ${SRCS} sync-manager-stub.c sync-adapter-stub.c)
TARGET_LINK_LIBRARIES(${this_target} ${PKGS_LDFLAGS} "-ldl -laul")


## SET LINKER FLAGS
#SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=/usr/lib")

SET_TARGET_PROPERTIES(${this_target}
	PROPERTIES
	VERSION ${FULLVER}
	SOVERSION ${VERSION_MAJOR}
)


CONFIGURE_FILE(${this_target}.pc.in ${CMAKE_SOURCE_DIR}/${this_target}.pc @ONLY)
#INSTALL(FILES ${CMAKE_SOURCE_DIR}/${this_target}.pc DESTINATION lib/pkgconfig)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${this_target}.pc DESTINATION ${LIBDIR}/pkgconfig)

#INSTALL(TARGETS ${this_target} DESTINATION lib)
INSTALL(TARGETS ${this_target} DESTINATION ${LIBDIR})


INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
		DESTINATION include
		FILES_MATCHING
		PATTERN "*_private.h" EXCLUDE
		PATTERN "*sync_adapter.h"
		PATTERN "*sync-error.h"
		PATTERN "*sync_manager_internal.h"
)
IF(_FEATURE_MOBILE_PROFILE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/include/mobile/sync_manager.h
		DESTINATION include
)
ELSE(_FEATURE_MOBILE_PROFILE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/include/wearable/sync_manager.h
		DESTINATION include
)
ENDIF(_FEATURE_MOBILE_PROFILE)


#INSTALL(DIRECTORY ${LIBRARY_OUTPUT_PATH}/ DESTINATION lib
INSTALL(DIRECTORY ${LIBRARY_OUTPUT_PATH}/ DESTINATION ${LIBDIR}
		FILES_MATCHING PATTERN "*core-sync-client*.so*"
		PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
					GROUP_EXECUTE GROUP_READ
					WORLD_EXECUTE WORLD_READ)
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS} "-ldl")
#INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})

