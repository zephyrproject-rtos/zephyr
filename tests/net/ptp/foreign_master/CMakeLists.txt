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
