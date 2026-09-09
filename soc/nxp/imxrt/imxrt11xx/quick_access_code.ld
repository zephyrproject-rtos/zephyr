/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The MCUX HAL places selected functions (e.g. fsl_romapi.c) into a
 * custom "CodeQuickAccess" input section via AT_QUICKACCESS_SECTION_CODE.
 * When it is left as an orphan section the linker also emits an orphan
 * ".ARM.exidxCodeQuickAccess" unwind-index table (kept whenever
 * CONFIG_ARCH_STACKWALK / CONFIG_FRAME_POINTER is set, e.g. sample.perf).
 * The R_ARM_PREL31 relocation in that exidx entry is only 31-bit PC
 * relative and overflows when the orphan code and its exidx table are
 * placed more than 1 GB apart, producing:
 *   relocation truncated to fit: R_ARM_PREL31 against `CodeQuickAccess'
 * Collecting the section into the image ROM region keeps the code and
 * its exidx entry together and resolves the overflow.
 */
SECTION_PROLOGUE(.code_quick_access,,)
{
	. = ALIGN(4);
	KEEP(*(CodeQuickAccess))
	. = ALIGN(4);
} GROUP_LINK_IN(ROMABLE_REGION)
