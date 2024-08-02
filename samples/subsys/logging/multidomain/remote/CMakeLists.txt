#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(logging_multidomain_remote)

target_sources(app PRIVATE src/main.c)

if (CONFIG_IPC_SERVICE)
  target_sources(app PRIVATE src/ipc_service.c)
endif()
