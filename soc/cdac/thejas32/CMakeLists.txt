# Copyright (c) 2026 Anuj Deshpande
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  start.S
  soc_irq.S
  soc_irq.c
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
