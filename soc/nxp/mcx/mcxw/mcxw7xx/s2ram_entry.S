/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(nxp_mcxw7x_s2ram_resume)
GTEXT(nxp_mcxw7x_s2ram_entry)
SECTION_FUNC(TEXT, nxp_mcxw7x_s2ram_entry)
	cpsid i
	movs r0, #0
	msr control, r0
	isb
	msr msplim, r0
	msr psplim, r0
	b nxp_mcxw7x_s2ram_resume
