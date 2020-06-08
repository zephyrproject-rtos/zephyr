# SPDX-License-Identifier: Apache-2.0

zephyr_sources(ctf_top.c)

zephyr_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_include_directories(.)
