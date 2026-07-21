# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c rom_header.S)

zephyr_linker_sources(ROM_START SORT_KEY 000romheader rom_header.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/riscv/common/linker.ld CACHE INTERNAL "")
