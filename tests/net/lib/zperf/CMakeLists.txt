# SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zperf_api)

target_sources(app PRIVATE
  src/main.c
  )

target_sources_ifdef(CONFIG_NET_ZPERF_IPERF3 app PRIVATE
  src/iperf3_proto.c
  src/iperf3_server.c
  )

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/zperf
  )
