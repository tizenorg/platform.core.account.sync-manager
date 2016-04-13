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
PROJECT("sync-manager")
ADD_DEFINITIONS("-flto")
IF(NOT DEFINED PACKAGE_NAME)
	SET(PACKAGE_NAME "sync-manager")
ENDIF(NOT DEFINED PACKAGE_NAME)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
IF(NOT DEFINED BINDIR)
	SET(BINDIR"${PREFIX}/bin")
ENDIF(NOT DEFINED BINDIR)

ADD_SUBDIRECTORY(src/sync-service)
ADD_SUBDIRECTORY(src/sync-client)
