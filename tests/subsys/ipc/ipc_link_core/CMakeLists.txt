# SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_link_core)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/ipc/ipc_service/lib)
