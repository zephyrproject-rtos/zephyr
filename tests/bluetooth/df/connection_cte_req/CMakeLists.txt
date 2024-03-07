#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_df_connection_cte_req)

FILE(GLOB app_sources src/*.c)
SET(common_sources ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/bt_common.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/bt_conn_common.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/../common/src/radio_df_stub.c)

target_sources(app PRIVATE ${common_sources} ${app_sources})
target_include_directories(app PRIVATE
			   ${CMAKE_CURRENT_SOURCE_DIR}/../common/include)

zephyr_library_include_directories(
    ${ZEPHYR_BASE}/subsys/bluetooth/
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
)

if(CONFIG_SOC_COMPATIBLE_NRF)
  zephyr_library_include_directories(
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
    ${ZEPHYR_BASE}/subsys/bluetooth/hci/nordic
  )
elseif(CONFIG_SOC_OPENISA_RV32M1)
  zephyr_library_include_directories(
    ${ZEPHYR_BASE}/bluetooth/controller/ll_sw/openisa
    ${ZEPHYR_BASE}/bluetooth/hci/openisa
  )
endif()
