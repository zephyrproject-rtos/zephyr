# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pcie_endpoint)

target_sources(app PRIVATE src/main.c)

if(PCIE_TEST_FAIL_CTRL_B)
  target_compile_definitions(app PRIVATE PCIE_TEST_FAIL_CTRL_B=1)
endif()

if(CONFIG_MMU)
  target_link_options(app PUBLIC
    "-Wl,--wrap=k_mem_map_phys_bare"
    "-Wl,--wrap=k_mem_unmap_phys_bare"
  )
endif()
