# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")

zephyr_sources(
  soc_irq.S
  )

add_subdirectory(s400)
add_subdirectory(s420)
