# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zperf)

target_sources(app PRIVATE
  src/shell_utils.c
  src/zperf_session.c
  src/zperf_shell.c
  src/zperf_udp_receiver.c
  src/zperf_udp_uploader.c
  src/zperf_tcp_receiver.c
  src/zperf_tcp_uploader.c
  )

target_include_directories(app PRIVATE
  $ENV{ZEPHYR_BASE}/subsys/net/ip
  )
