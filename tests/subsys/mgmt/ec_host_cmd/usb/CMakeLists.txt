# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ec_host_cmd_usb)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  ${app_sources}
  ${ZEPHYR_BASE}/subsys/mgmt/ec_host_cmd/backends/ec_host_cmd_backend_usb.c
)
