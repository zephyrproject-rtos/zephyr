# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_transport_retry)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

target_compile_definitions(app PRIVATE
  CONFIG_PTP_LOG_LEVEL=3
  CONFIG_PTP_DSCP_VALUE=0
  CONFIG_PTP_IEEE_802_3_PROTOCOL=1
  CONFIG_PTP_UDP_IPv4_PROTOCOL=0
  CONFIG_PTP_UDP_IPv6_PROTOCOL=0
)
