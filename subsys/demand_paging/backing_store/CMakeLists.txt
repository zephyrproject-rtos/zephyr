# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

if(NOT DEFINED CONFIG_BACKING_STORE_CUSTOM)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_BACKING_STORE_RAM   ram.c)

  zephyr_library_sources_ifdef(
    CONFIG_BACKING_STORE_QEMU_X86_TINY_FLASH
    backing_store_qemu_x86_tiny.c
    )
endif()
