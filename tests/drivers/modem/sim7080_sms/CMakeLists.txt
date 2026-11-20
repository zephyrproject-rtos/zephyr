# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sim7080_sms)

target_sources(app PRIVATE src/main.c)

# src/main.c includes the driver source to reach its static functions
target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/drivers/modem/vendor_standalone/simcom/sim7080
  ${ZEPHYR_BASE}/drivers/modem
  ${ZEPHYR_BASE}/subsys/net/ip
  ${ZEPHYR_BASE}/subsys/net/lib/sockets
)
