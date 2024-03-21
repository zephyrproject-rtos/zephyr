# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(integration)

target_sources(app PRIVATE src/common.c)
target_sources(app PRIVATE src/classic.c)
target_sources(app PRIVATE src/transceiver.c)
target_sources(app PRIVATE src/utilities.c)
target_sources_ifdef(CONFIG_CAN_FD_MODE app PRIVATE src/canfd.c)
target_sources_ifdef(CONFIG_CAN_STATS app PRIVATE src/stats.c)
