# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_tlv_mgmt)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/subsys/net/lib/ptp/tlv.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)
