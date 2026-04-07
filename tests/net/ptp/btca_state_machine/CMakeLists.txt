# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_btca_state_machine)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/btca.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/state_machine.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

target_compile_definitions(app PRIVATE
  CONFIG_PTP_LOG_LEVEL=3
  CONFIG_PTP_DISABLED_PRESENT=1
  CONFIG_PTP_FAULTY_PRESENT=1
  CONFIG_PTP_LISTENING_PRESENT=1
  CONFIG_PTP_PRE_TIME_TRANSMITTER_PRESENT=1
  CONFIG_PTP_UNCALIBRATED_PRESENT=1
)
