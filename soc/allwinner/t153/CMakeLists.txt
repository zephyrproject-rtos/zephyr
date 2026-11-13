# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT
  "${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld"
  CACHE INTERNAL ""
)

zephyr_sources(
  irq_context.S
  soc.c
)

zephyr_linker_sources_ifdef(CONFIG_OPENAMP_RSC_TABLE ROM_SECTIONS
  resource_table.ld
)
