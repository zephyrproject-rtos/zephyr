#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(os_mgmt_info)

FILE(GLOB app_sources
	src/*.c
)

target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/transport/include/mgmt/mcumgr/transport/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/grp/os_mgmt/include/)

if(DEFINED CONFIG_BUILD_DATE_TIME_TEST)
  set(TEST_DATE_TIME_DIR ${PROJECT_BINARY_DIR}/test)
  file(MAKE_DIRECTORY ${TEST_DATE_TIME_DIR})
  file(WRITE ${TEST_DATE_TIME_DIR}/test_date.c "/* Auto generated file, do not edit */\n#include <stdint.h>\nuint8_t *test_date_time = __TIMESTAMP__;")
  target_sources(app PRIVATE ${TEST_DATE_TIME_DIR}/test_date.c)
endif()
