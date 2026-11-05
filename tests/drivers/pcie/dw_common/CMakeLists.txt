# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_dw_common)

# The common DesignWare driver is not a standalone library, so compile its one
# source file into the test application and point the include path at its
# private header.
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/pcie/controller)
target_compile_definitions(app PRIVATE CONFIG_PCIE_LOG_LEVEL=0)
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/drivers/pcie/controller/pcie_dw_common.c
)
