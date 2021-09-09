# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(ti_cc13x2_cc26x2_system_off)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
