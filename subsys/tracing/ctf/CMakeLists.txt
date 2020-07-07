# SPDX-License-Identifier: Apache-2.0

zephyr_sources(ctf_top.c)

zephyr_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

zephyr_include_directories(.)
