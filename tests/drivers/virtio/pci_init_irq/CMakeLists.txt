# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_pci_init_irq)

target_sources(app PRIVATE src/main.c)

# qemu_x86 links a strong arch_irq_enable(), so use linker wrapping to let
# the FFF fake observe calls without defining a duplicate strong symbol.
target_link_options(app PRIVATE "-Wl,--wrap=arch_irq_enable")
