# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bridge_stp)

set(STP_DIR ${ZEPHYR_BASE}/subsys/net/l2/ethernet/bridge/stp)

target_include_directories(app PRIVATE ${STP_DIR})
target_sources(app PRIVATE ${STP_DIR}/rstp_bpdu.c)
file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
