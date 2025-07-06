# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE} CACHE PATH "Zephyr base")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ZEPHYR_BASE}/cmake/modules)

find_package(Zephyr-sdk COMPONENTS LIST)
