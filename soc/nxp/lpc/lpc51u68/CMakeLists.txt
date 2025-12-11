#
# Copyright (c) 2021 metraTec GmbH
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
