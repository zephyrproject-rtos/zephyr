# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_pci_status)

target_sources(app PRIVATE src/main.c)

if(CONFIG_X86)
  zephyr_ld_options("-Wl,--wrap=pcie_conf_read" "-Wl,--wrap=pcie_conf_write")
endif()
