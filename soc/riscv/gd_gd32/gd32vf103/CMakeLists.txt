# Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(entry.S)
zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
