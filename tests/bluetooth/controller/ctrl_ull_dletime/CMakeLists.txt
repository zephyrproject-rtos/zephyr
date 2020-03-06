# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if (NOT BOARD STREQUAL unit_testing)
  message(FATAL_ERROR "This project can only be used with '-DBOARD=unit_testing'.")
endif()

#
# the following allows us to test the different PHY configurations
# by setting the appropriate flags in the yaml file
#
if(CONFIG_BT_CTLR_PHY)
  add_compile_definitions(CONFIG_BT_CTLR_PHY)
endif(CONFIG_BT_CTLR_PHY)

if(CONFIG_BT_CTLR_PHY_2M)
  add_compile_definitions(CONFIG_BT_CTLR_PHY_2M)
endif(CONFIG_BT_CTLR_PHY_2M)

if(CONFIG_BT_CTLR_PHY_CODED)
  add_compile_definitions(CONFIG_BT_CTLR_PHY_CODED)
endif(CONFIG_BT_CTLR_PHY_CODED)

if(CONFIG_BT_CTLR_DATA_LENGTH_MAX)
  add_compile_definitions(CONFIG_BT_CTLR_DATA_LENGTH_MAX=${CONFIG_BT_CTLR_DATA_LENGTH_MAX})
endif(CONFIG_BT_CTLR_DATA_LENGTH_MAX)

set(INCLUDE
  tests/bluetooth/controller/mock_ctrl/include
  include/bluetooth
  subsys/bluetooth
  subsys/bluetooth/controller
  subsys/bluetooth/controller/util
  subsys/bluetooth/controller/include
  subsys/bluetooth/controller/ll_sw
)

FILE(GLOB SOURCES
  src/*.c
)

project(bluetooth_ctrl_ull_conn)
find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})


#
# we test functionality of ull_conn, which is included directly in the test
# file so we do not list it here
#
FILE(GLOB ll_sw_sources
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/util/mem.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/util/memq.c
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_tx_queue.c
)

FILE(GLOB mock_sources
  ${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ll.c
  ${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ull.c
  ${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ull_master.c
  ${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ticker.c
  ${ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/mayfly.c
)

target_sources(testbinary PRIVATE ${ll_sw_sources} ${mock_sources})
