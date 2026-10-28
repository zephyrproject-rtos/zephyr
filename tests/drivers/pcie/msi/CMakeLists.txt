# Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_msi)

option(TEST_MSI_64BIT_PROVIDER "Provide a 64-bit MSI address backend" OFF)

target_sources(app PRIVATE
  src/main.c
  "${ZEPHYR_BASE}/drivers/pcie/host/msi.c"
)

# Isolate the MSI core with synthetic feature definitions and test providers.
target_compile_definitions(app PRIVATE
  CONFIG_PCIE_CONTROLLER=1
  CONFIG_PCIE_MSI_X=1
  CONFIG_PCIE_MSI_MULTI_VECTOR=1
)

if(TEST_MSI_64BIT_PROVIDER)
  target_compile_definitions(app PRIVATE TEST_MSI_64BIT_PROVIDER=1)
endif()
