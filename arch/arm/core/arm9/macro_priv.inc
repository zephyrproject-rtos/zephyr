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

#endif /* _ARM9_MACRO_PRIV_INC_ */
