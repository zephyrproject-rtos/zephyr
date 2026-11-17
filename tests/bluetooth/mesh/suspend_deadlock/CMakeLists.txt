# SPDX-FileCopyrightText: Copyright (c) 2026 Tarek Tarek
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_suspend_deadlock)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth/mesh)
