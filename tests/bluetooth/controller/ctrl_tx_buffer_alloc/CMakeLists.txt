# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if (NOT BOARD STREQUAL unit_testing)
  message(FATAL_ERROR "This project can only be used with '-DBOARD=unit_testing'.")
endif()

FILE(GLOB SOURCES
  src/*.c
)
if(CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM GREATER_EQUAL 0)
  add_compile_definitions(CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM=${CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM})
endif(CONFIG_BT_CTLR_LLCP_PER_CONN_TX_CTRL_BUF_NUM)

project(bluetooth_ull_llcp_tx_buffer_alloc)
find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})
include(${ZEPHYR_BASE}/tests/bluetooth/controller/common/defaults_cmake.txt)
target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources} ${common_sources})
