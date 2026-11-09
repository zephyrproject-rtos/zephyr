# Copyright (c) 2026 Takayuki Goto
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(soc.c)

# The AArch32 linker script is shared with the Cortex-A/R port; nothing in it
# is Cortex specific.
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
