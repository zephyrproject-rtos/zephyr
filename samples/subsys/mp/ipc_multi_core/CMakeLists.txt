# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mp_ipc_multi_core)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_MBOX app PRIVATE src/mbox_mock.c)
