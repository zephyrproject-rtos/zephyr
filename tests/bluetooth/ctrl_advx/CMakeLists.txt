# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bluetooth_advx)

zephyr_include_directories(
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
  )

target_sources(app PRIVATE src/main.c)
