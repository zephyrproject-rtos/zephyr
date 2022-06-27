# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT BOARD STREQUAL unit_testing)
  message(FATAL_ERROR "This project can only be used with '-DBOARD=unit_testing'.")
endif()

FILE(GLOB SOURCES
  src/*.c
)
if(CONFIG_BT_CTLR_PHY_CODED)
  add_compile_definitions(CONFIG_BT_CTLR_PHY_CODED)
endif(CONFIG_BT_CTLR_PHY_CODED)

if(CONFIG_BT_CTLR_PHY)
  add_compile_definitions(CONFIG_BT_CTLR_PHY)
endif(CONFIG_BT_CTLR_PHY)

project(bluetooth_ull_llcp_data_length_update)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)
if(NOT CONFIG_BT_CTLR_PHY)
list(REMOVE_ITEM ll_sw_sources ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_llcp_phy.c)
endif()
target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources} ${common_sources})
