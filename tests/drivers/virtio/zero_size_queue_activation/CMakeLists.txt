# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(virtio_zero_size_queue_activation)

target_sources_ifdef(CONFIG_VIRTIO_MMIO app PRIVATE src/mmio.c)
target_sources_ifdef(CONFIG_VIRTIO_PCI app PRIVATE src/pci.c)

if(CONFIG_VIRTIO_PCI)
  endif()
