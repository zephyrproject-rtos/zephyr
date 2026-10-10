# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_l2_framing)

# src/main.c #includes layer2_data_link.c so that the test can reach the file's
# static state machine table and its private variables.  The KNX subsystem
# itself is NOT enabled (CONFIG_KNX_STACK=n) — knx_core.c's SYS_INIT would
# start the L2 threads and the NCN5130 driver, which this test does not want.
target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/knx)

# Kconfig symbols layer2_data_link.c needs at file scope.  They come from the
# KNX subsystem's Kconfig, which is not part of this build (CONFIG_KNX_STACK=n).
# The thread sizes/priorities are only used by L_Init(), which the test never
# calls, but they must still compile.
target_compile_definitions(app PRIVATE
    CONFIG_KNX_STACK_LOG_LEVEL=0
    CONFIG_KNX_L2_TX_QUEUE_DEPTH_HIGH=2
    CONFIG_KNX_L2_TX_QUEUE_DEPTH_LOW=4
    CONFIG_KNX_L2_RX_QUEUE_DEPTH=8
    CONFIG_KNX_L2_TX_STACK_SIZE=2048
    CONFIG_KNX_L2_RX_STACK_SIZE=2048
    CONFIG_KNX_L2_TX_PRIO=5
    CONFIG_KNX_L2_RX_PRIO=10
)
