/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#if (CONFIG_FLASH_MCUX_FLEXSPI_XIP || CONFIG_FLASH_MCUX_FLEXSPI_NOR)
/*
 * With CONFIG_ARCH_STACKWALK the compiler emits .ARM.exidx.ramfunc /
 * .ARM.extab.ramfunc unwind entries for every __ramfunc function.
 * FlexSPI flash sits at 0x90000000 and SRAM at ~0x20000000; the
 * 1.75 GB gap exceeds the +-1 GB (31-bit signed) R_ARM_PREL31 range,
 * causing a link error.
 *
 * Discard these entries before the generic *(.ARM.exidx*) wildcard
 * in the ARM template can claim them.
 * zephyr_linker_sources(SECTIONS ...) inserts snippets after that
 * wildcard so it cannot be used here.
 */
SECTIONS
{
	/DISCARD/ : { *(.ARM.exidx.ramfunc*) *(.ARM.extab.ramfunc*) }
}
#endif

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
