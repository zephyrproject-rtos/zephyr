# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

if (CONFIG_ICM42688_EMUL)
target_sources(app PRIVATE src/emul_main.c)
else ()
target_sources(app PRIVATE src/hw_main.c)
endif()
