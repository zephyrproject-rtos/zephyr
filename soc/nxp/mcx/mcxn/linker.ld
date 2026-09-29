/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#if (CONFIG_FLASH_MCUX_FLEXSPI_XIP || CONFIG_FLASH_MCUX_FLEXSPI_NOR)
/*
 * Code marked __ramfunc is relocated into .ramfunc in SRAM.  With
 * CONFIG_ARCH_STACKWALK (-funwind-tables) the compiler also emits
 * .ARM.exidx.ramfunc / .ARM.extab.ramfunc unwind entries for each
 * such function.  When ROMABLE_REGION is in external FlexSPI flash
 * those entries would be gathered into the main .ARM.exidx table there,
 * but the R_ARM_PREL31 relocation back to SRAM cannot be encoded because
 * the distance exceeds the signed 31-bit range ("relocation truncated to
 * fit: R_ARM_PREL31 against .ramfunc").
 *
 * The discard rule below must appear in a SECTIONS block that precedes
 * the standard ARM linker template so it fires before the generic
 * *(.ARM.exidx*) wildcard in that template can claim these entries.
 * Using the zephyr_linker_sources(SECTIONS ...) mechanism places the
 * rule too late (after the wildcard) and has no effect.
 */
SECTIONS
{
	/DISCARD/ : { *(.ARM.exidx.ramfunc*) *(.ARM.extab.ramfunc*) }
}
#endif

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
