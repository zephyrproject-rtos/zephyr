#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_df_connectionless_cte_tx)

FILE(GLOB app_sources src/*.c)
SET(common_sources ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/radio_df_stub.c)

target_sources(app PRIVATE ${common_sources} ${app_sources})

# Include paths to allow access to functions implemented in BLE controller
target_include_directories(app PRIVATE
			   ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)
target_include_directories(app PRIVATE
			   ${ZEPHYR_BASE}/subsys/bluetooth/controller)
target_include_directories(app PRIVATE
			   ${ZEPHYR_BASE}/subsys/bluetooth/controller/include)
target_include_directories(app PRIVATE
			   ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw)
target_include_directories(app PRIVATE
			   ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic)
target_include_directories(app PRIVATE
			   ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic/lll)
