# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)

zephyr_sources(
  soc.c
  soc_irq.c
)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
