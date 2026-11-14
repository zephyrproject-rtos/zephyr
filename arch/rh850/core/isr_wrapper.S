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

GTEXT(z_rh850_isr_wrapper)
GTEXT(z_rh850_irq_exit)
GTEXT(z_rh850_frame_restore)
GTEXT(RH850_C_SYM(z_irq_spurious))
GDATA(RH850_C_SYM(_kernel))
GDATA(RH850_C_SYM(_sw_isr_table))
#ifdef CONFIG_MULTITHREADING
GTEXT(RH850_C_SYM(z_get_next_switch_handle))
#endif
#ifdef CONFIG_FXU
GTEXT(RH850_C_SYM(z_rh850_fxu_switch))
#endif
#ifdef CONFIG_TRACING_ISR
GTEXT(RH850_C_SYM(sys_trace_isr_enter))
GTEXT(RH850_C_SYM(sys_trace_isr_exit))
#endif

/* ISPR holds back requests of the same and lower priority until EIRET */
SECTION_FUNC(TEXT, z_rh850_isr_wrapper)
	z_rh850_frame_save
	z_rh850_irq_enter

#ifdef CONFIG_TRACING_ISR
	jarl	RH850_C_SYM(sys_trace_isr_enter), lp
#endif

	rh850_stsr	RH850_SR_EIIC, r10
	andi	0xffff, r10, r10
	addi	-RH850_EIIC_EIINT, r10, r10
	mov	CONFIG_NUM_IRQS, r11
	cmp	r11, r10
	bnl	3f

	shl	3, r10
	mov	hilo(RH850_C_SYM(_sw_isr_table)), r11
	add	r11, r10
	ld.w	0[r10], r6
	ld.w	4[r10], r11
	ei
	jarl	[r11], lp

#ifdef CONFIG_TRACING_ISR
	jarl	RH850_C_SYM(sys_trace_isr_exit), lp
#endif
	di

/* EIRET clears the ISPR bit only when PSW.EP = 0 */
z_rh850_irq_exit:
	ld.w	___cpu_t_nested_OFFSET[r21], r10
#ifdef CONFIG_MULTITHREADING
	cmp	1, r10
	bne	1f
#ifdef CONFIG_FXU
	ld.w	___cpu_t_current_OFFSET[r21], r22
#endif
	mov	r20, r6
	jarl	RH850_C_SYM(z_get_next_switch_handle), lp
#ifdef CONFIG_FXU
	mov	r10, r23
	mov	r22, r6
	ld.w	___cpu_t_current_OFFSET[r21], r7
	jarl	RH850_C_SYM(z_rh850_fxu_switch), lp
	mov	r23, r10
#endif
	st.w	r0, ___cpu_t_nested_OFFSET[r21]
	mov	r10, sp
	jr	z_rh850_frame_restore
1:
#endif
	add	-1, r10
	st.w	r10, ___cpu_t_nested_OFFSET[r21]
	mov	r20, sp
	jr	z_rh850_frame_restore

3:	mov	r0, r6
	jarl	RH850_C_SYM(z_irq_spurious), lp
	br	z_rh850_irq_exit
