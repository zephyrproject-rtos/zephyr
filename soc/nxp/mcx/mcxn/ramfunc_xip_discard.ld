/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Code marked __ramfunc is relocated into .ramfunc in SRAM.  With
 * CONFIG_ARCH_STACKWALK (-funwind-tables) the compiler also emits
 * .ARM.exidx.ramfunc / .ARM.extab.ramfunc unwind entries for each
 * such function.  When the ROMABLE_REGION is in external FlexSPI flash
 * those entries would be gathered into the main .ARM.exidx table there,
 * but the R_ARM_PREL31 relocation back to SRAM cannot be encoded because
 * the distance exceeds the signed 31-bit range ("relocation truncated to
 * fit: R_ARM_PREL31 against .ramfunc").  The Cortex-M stack unwinder does
 * not consume these tables, so discard them.
 */
/DISCARD/ :
{
	*(.ARM.exidx.ramfunc*)
	*(.ARM.extab.ramfunc*)
}
