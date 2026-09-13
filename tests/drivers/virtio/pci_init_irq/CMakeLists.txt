# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_pci_init_irq)

target_sources(app PRIVATE src/main.c)
target_link_options(app PRIVATE "-Wl,--wrap=arch_irq_enable")
