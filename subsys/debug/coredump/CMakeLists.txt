# Copyright (c) 2020 Intel Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_library_sources(
  coredump_core.c
  coredump_memory_regions.c
  )

zephyr_library_sources_ifdef(
  CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING
  coredump_backend_logging.c
  )

zephyr_library_sources_ifdef(
  CONFIG_DEBUG_COREDUMP_BACKEND_FLASH_PARTITION
  coredump_backend_flash_partition.c
  )
