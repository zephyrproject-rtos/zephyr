/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ARM9_MACRO_PRIV_INC_
#define _ARM9_MACRO_PRIV_INC_

#include <zephyr/arch/cpu.h>
#include <offsets_short.h>

/*
 * ARMv5 has neither TPIDRURO nor MPIDR, and this port is uniprocessor only,
 * so the "current CPU" is always &_kernel.cpus[0].
 */
.macro get_cpu rreg0
	ldr \rreg0, =_kernel
	add \rreg0, \rreg0, #___kernel_t_cpus_OFFSET
.endm

/*
 * Disable IRQs. ARMv5 has no CPS/CPSID, so CPSR has to be read back,
 * modified and written.  Clobbers \rreg0.
 */
.macro z_armv5_irq_disable rreg0
	mrs \rreg0, cpsr
	orr \rreg0, \rreg0, #I_BIT
	msr cpsr_c, \rreg0
.endm

/* Enable IRQs.  Clobbers \rreg0. */
.macro z_armv5_irq_enable rreg0
	mrs \rreg0, cpsr
	bic \rreg0, \rreg0, #I_BIT
	msr cpsr_c, \rreg0
.endm

/*
 * Exception entry.
 *
 * Entered in one of the exception modes (IRQ/FIQ/ABT/UND/SVC) with:
 *   - lr   : already corrected return address of the interrupted context
 *   - spsr : CPSR of the interrupted context
 *   - sp   : points at this mode's 3-word scratch area (see stacks.c).
 *
 * ARMv5 has no SRSDB/CPS, so the classic "vector stub" sequence is used:
 * r0, lr_<exc> and spsr_<exc> are parked in the per-mode scratch area, the
 * core is switched to system mode and the exception stack frame is then
 * built on the system mode (i.e. thread or ISR) stack.
 *
 * On return: r0 points at the exception stack frame, and the CPU is in
 * system mode with IRQ and FIQ masked.
 *
 * NOTE: threads and ISRs always execute in system mode in this port, so the
 * lr saved in the frame below (r14_sys) really is the interrupted context's
 * lr.
 */
.macro z_armv5_enter_exc
	stmia	sp, {r0, lr}		/* scratch[0] = r0, scratch[1] = pc */
	mrs	lr, spsr
	str	lr, [sp, #8]		/* scratch[2] = interrupted CPSR */
	mov	r0, sp			/* carry the scratch pointer in r0 */

	msr	cpsr_c, #(MODE_SYS | I_BIT | F_BIT)

	/* Build struct __basic_sf: r0, r1, r2, r3, r12, lr, pc, xpsr */
	sub	sp, sp, #32
	str	r1, [sp, #4]
	str	r2, [sp, #8]
	str	r3, [sp, #12]
	str	r12, [sp, #16]
	str	lr, [sp, #20]
	ldmia	r0, {r1, r2, r3}	/* r1 = r0, r2 = pc, r3 = xpsr */
	str	r1, [sp, #0]
	str	r2, [sp, #24]
	str	r3, [sp, #28]

	mov	r0, sp

	/* increment exception depth */
	get_cpu	r2
	ldrb	r1, [r2, #_cpu_offset_to_exc_depth]
	add	r1, r1, #1
	strb	r1, [r2, #_cpu_offset_to_exc_depth]
.endm

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
/*
 * Size of what z_armv5_esf_extend pushes below the basic frame: struct
 * _callee_saved (r4-r11, sp, lr), then a copy of the basic frame preceded
 * by struct __extra_esf_info. fault.c checks these against the C layout.
 */
#define ARMV5_CALLEE_SIZE	40
#define ARMV5_EXTRA_INFO_SIZE	12
#define ARMV5_BASIC_SF_SIZE	32

/*
 * Turn the basic frame built by z_armv5_enter_exc into a complete
 * struct arch_esf for the fault handlers.
 *
 * As on Cortex-M, callee.psp is the address of the basic frame, so the
 * stack pointer of the interrupted context is callee.psp + 32.
 *
 * Entered with r0 = sp = basic frame. The IRQ path and the exception return
 * only ever see the basic frame, so the extended one is built as a copy
 * below it rather than changing the frame layout everywhere.
 *
 * On return r0 = sp = struct arch_esf. Clobbers r1-r10.
 */
.macro z_armv5_esf_extend
	mov	r1, r0				/* psp: the basic frame */
	ldr	r2, [r0, #20]			/* lr before the exception */
	stmdb	sp!, {r1, r2}
	stmdb	sp!, {r4-r11}
	mov	r10, sp				/* struct _callee_saved */

	ldmia	r0, {r1-r8}
	stmdb	sp!, {r1-r8}			/* copy of the basic frame */

	mov	r1, r10				/* extra_info.callee */
	mov	r2, r0				/* extra_info.msp */
	mov	r3, #0				/* extra_info.exc_return */
	stmdb	sp!, {r1-r3}

	mov	r0, sp
.endm

/*
 * Undo z_armv5_esf_extend with sp = struct arch_esf: write the basic frame
 * and r4-r11, which a handler may have changed, back to where the exception
 * return expects them. Leaves sp at the basic frame. Clobbers r0-r9.
 */
.macro z_armv5_esf_unextend
	add	r0, sp, #ARMV5_EXTRA_INFO_SIZE
	ldmia	r0, {r1-r8}
	add	r9, sp, #(ARMV5_EXTRA_INFO_SIZE + ARMV5_BASIC_SF_SIZE + ARMV5_CALLEE_SIZE)
	stmia	r9, {r1-r8}
	add	sp, sp, #(ARMV5_EXTRA_INFO_SIZE + ARMV5_BASIC_SF_SIZE)
	ldmia	sp!, {r4-r11}
	add	sp, sp, #8			/* sp and lr */
.endm
#else
.macro z_armv5_esf_extend
.endm

.macro z_armv5_esf_unextend
.endm
#endif /* CONFIG_EXTRA_EXCEPTION_INFO */

#endif /* _ARM9_MACRO_PRIV_INC_ */
