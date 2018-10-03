cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2018 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)
