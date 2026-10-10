# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_pci_init_irq)

target_include_directories(testbinary PRIVATE
  ${ZEPHYR_BASE}/drivers/virtio
)

target_sources(testbinary PRIVATE src/main.c)
