# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
# Ahead of the vendor's include directories: the test's lll_meta.h and lll_conn_meta.h replace the
# vendor's, which #error when the options are enabled.
include_directories("./src")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_ctrl_pdu_meta)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/controller/)

if(CONFIG_SOC_COMPATIBLE_NRF)
  zephyr_library_include_directories(
    ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
    ${ZEPHYR_BASE}/subsys/bluetooth/hci/nordic
  )
endif()

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/controller
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
)

file(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
