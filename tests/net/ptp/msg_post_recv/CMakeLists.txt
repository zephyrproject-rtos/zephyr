# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_msg_post_recv)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/msg.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

target_compile_definitions(app PRIVATE
  CONFIG_PTP_LOG_LEVEL=3
  CONFIG_PTP_MSG_POLL_SIZE=4
)
