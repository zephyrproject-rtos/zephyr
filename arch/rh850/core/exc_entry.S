/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/rh850/sysreg.h>
#include <offsets_short.h>
#include <kernel_arch_data.h>
#include "asm_macros.inc"

GTEXT(z_rh850_exc_fe)
GTEXT(z_rh850_exc_ei)
GTEXT(z_rh850_exc_ei_saved)
GTEXT(RH850_C_SYM(z_rh850_fault))
GTEXT(z_rh850_irq_exit)

SECTION_FUNC(TEXT, z_rh850_exc_fe)
	z_rh850_frame_save fe=1, exc=1
	rh850_stsr	RH850_SR_FEIC, r7
	mov	RH850_EXC_LEVEL_FE, r8
	br	1f

SECTION_FUNC(TEXT, z_rh850_exc_ei)
	z_rh850_frame_save exc=1
z_rh850_exc_ei_saved:
	rh850_stsr	RH850_SR_EIIC, r7
	mov	RH850_EXC_LEVEL_EI, r8
1:	z_rh850_irq_enter
	mov	r20, r6
	jarl	RH850_C_SYM(z_rh850_fault), lp
	jr	z_rh850_irq_exit
