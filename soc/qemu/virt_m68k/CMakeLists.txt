# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/m68k/linker.ld CACHE INTERNAL "")
