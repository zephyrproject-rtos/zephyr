# Copyright (c) 2024 Michael Hope
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc_irq.S
  vector.S
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
