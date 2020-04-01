# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

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


include_directories(
        $ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/include
        $ENV{ZEPHYR_BASE}/tests/bluetooth/controller/common/include
        $ENV{ZEPHYR_BASE}/include/bluetooth
        $ENV{ZEPHYR_BASE}/subsys/bluetooth
        $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller
        $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/util
        $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/include
        $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
)

#
# we test functionality of ull_conn, which is included directly in the test file
# so we do not list it here
#
FILE(GLOB ll_sw_sources
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/util/mem.c
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/util/memq.c
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_tx_queue.c

)

FILE(GLOB mock_sources
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ll.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ull.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ull_master.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/ticker.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/mock_ctrl/src/mayfly.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/common/src/helper_pdu.c
	$ENV{ZEPHYR_BASE}/tests/bluetooth/controller/common/src/helper_util.c

)

FILE(GLOB app_sources
        src/*.c
)

if(BOARD STREQUAL unit_testing)
	project(bluetooth_ctrl_ull_conn)
	list(APPEND SOURCES ${app_sources} ${ll_sw_sources} ${mock_sources})
	# we can not use find_package yet
	#	find_package(ZephyrUnittest HINTS $ENV{ZEPHYR_BASE})
	include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
else()
#
# Note: not working
#
	include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
	target_sources(app PRIVATE ${app_sources} ${ll_sw_sources} ${mock_sources})
endif()
