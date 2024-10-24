# Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(entry.S)
zephyr_sources(soc.c)
zephyr_sources(soc_irq.S)

zephyr_include_directories(.)

zephyr_linker_sources(ROM_START SORT_KEY 0x0 init.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
