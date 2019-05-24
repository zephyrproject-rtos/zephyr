# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(cpp_synchronization)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/include/cmsis_rtos_v2)
target_sources(app PRIVATE src/main.c)
