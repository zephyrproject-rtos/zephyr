#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_PLATFORM_SPECIFIC_INIT gcc/startup_LPC54114_cm4.S)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )
