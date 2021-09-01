#
# Copyright (c) 2021 Dung.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(soc.c)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )