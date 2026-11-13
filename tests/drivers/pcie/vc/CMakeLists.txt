# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_vc)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/drivers/pcie/host/vc.c
)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/pcie/host)
