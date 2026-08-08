# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_ep_emul_gate)

target_sources(app PRIVATE src/main.c)
