# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_clock_decision)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/clock.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

target_compile_definitions(app PRIVATE
  CONFIG_PTP_LOG_LEVEL=3
  CONFIG_PTP_NUM_PORTS=8
  CONFIG_PTP_CLOCK_TYPE=0
  CONFIG_PTP_TIME_RECEIVER_ONLY=0
  CONFIG_PTP_CLOCK_ACCURACY=0x31
  CONFIG_PTP_PRIORITY1=128
  CONFIG_PTP_PRIORITY2=128
)
