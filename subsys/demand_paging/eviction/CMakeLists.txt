# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

add_definitions(-D__ZEPHYR_SUPERVISOR__)

include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

if(NOT DEFINED CONFIG_EVICTION_CUSTOM)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_EVICTION_NRU            nru.c)
endif()
