# Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
# Copyright 2024-2025 openEuler SIG-Zephyr
# SPDX-License-Identifier: Apache-2.0

#Add env for build zvm
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zvm)

target_sources(app PRIVATE src/main.c)
