# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_foreign_master)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/port.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

target_compile_definitions(app PRIVATE
  CONFIG_PTP_LOG_LEVEL=3
  CONFIG_PTP_NUM_PORTS=1
  CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_FEATURE=1
  CONFIG_PTP_FOREIGN_TIME_TRANSMITTER_RECORD_SIZE=5
  CONFIG_PTP_MIN_DELAY_REQ_LOG_INTERVAL=0
  CONFIG_PTP_ANNOUNCE_LOG_INTERVAL=1
  CONFIG_PTP_ANNOUNCE_RECV_TIMEOUT=3
  CONFIG_PTP_SYNC_LOG_INTERVAL=0
  CONFIG_PTP_MIN_PDELAY_REQ_LOG_INTERVAL=0
)
