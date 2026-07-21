# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE} CACHE PATH "Zephyr base")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ZEPHYR_BASE}/cmake/modules)

include(git)

if(VERSION_TYPE STREQUAL KERNEL)
  set(BUILD_VERSION_NAME BUILD_VERSION)
else()
  set(BUILD_VERSION_NAME ${VERSION_TYPE}_BUILD_VERSION)
endif()

if(NOT DEFINED ${BUILD_VERSION_NAME})
  cmake_path(GET VERSION_FILE PARENT_PATH work_dir)
  git_describe(${work_dir} ${BUILD_VERSION_NAME})
endif()

include(${ZEPHYR_BASE}/cmake/modules/version.cmake)
file(READ ${ZEPHYR_BASE}/version.h.in version_content)
string(CONFIGURE "${version_content}" version_content)
string(CONFIGURE "${version_content}" version_content)
file(WRITE ${OUT_FILE} "${version_content}")
