# Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_static_vrings)

target_include_directories(
  app PRIVATE "${ZEPHYR_BASE}/subsys/ipc/ipc_service/backends")
target_sources(app PRIVATE src/main.c src/mbox.c)
