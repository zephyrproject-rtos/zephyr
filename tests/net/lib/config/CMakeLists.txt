# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(config)

# For the SNTP packet struct definitions
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/sntp)

target_sources(app PRIVATE src/test_util.c)

# The two scenarios cannot share a binary: the resync tests need the periodic
# work the runtime-server tests need kept out of the way.
if(CONFIG_NET_CONFIG_SNTP_INIT_RESYNC)
  target_sources(app PRIVATE src/resync.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
