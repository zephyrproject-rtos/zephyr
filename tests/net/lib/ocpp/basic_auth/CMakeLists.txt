# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ocpp_basic_auth_test)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ocpp
)
target_sources(app PRIVATE src/main.c)
