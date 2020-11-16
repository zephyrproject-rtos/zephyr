/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/sparc/sparc.h>
#include "stack_offsets.h"

GTEXT(__sparc_trap_interrupt)
GTEXT(__sparc_trap_irq_offload)

/*
 * Interrupt trap handler
 *
 * - IU state is saved and restored
 *
 * On entry:
 * %l0: psr (set by trap code)
 * %l1: pc
 * %l2: npc
 * %l3: SPARC interrupt request level (bp_IRL)
 * %fp: %sp of current register window at trap time
 *
 * This module also implements the IRQ offload support. The handling is the
 * same as for asynchronous maskable interrupts, with the following exceptions:
 * - Do not re-execute the causing (ta) instruction at trap exit.
 * - A dedicated interrupt request level (0x8d) is used.
 * - z_sparc_enter_irq() knows how to interpret this interrupt request level.
 */
SECTION_SUBSEC_FUNC(TEXT, __sparc_trap_interrupt, __sparc_trap_irq_offload)
	/* Preparation in the case of synchronous IRQ offload. */
	mov	%l2, %l1
	add	%l2, 4, %l2
	set	0x8d, %l3

__sparc_trap_interrupt:
	/* %g2, %g3 are used at manual window overflow so save temporarily */
	mov	%g2, %l4
	mov	%g3, %l5

	/* We may have trapped into the invalid window. If so, make it valid. */
	rd	%wim, %g2
	srl	%g2, %l0, %g3
	cmp	%g3, 1
	bne	.Lwodone
	 nop

	/* Do the window overflow. */
	sll	%g2, (CONFIG_SPARC_NWIN-1), %g3
	srl	%g2, 1, %g2
	or	%g2, %g3, %g2

	/* Enter window to save. */
	save
	/* Install new wim calculated above. */
	mov	%g2, %wim
	nop
	nop
	nop
	/* Put registers on the dedicated save area of the ABI stack frame. */
	std	%l0, [%sp + 0x00]
	std	%l2, [%sp + 0x08]
	std	%l4, [%sp + 0x10]
	std	%l6, [%sp + 0x18]
	std	%i0, [%sp + 0x20]
	std	%i2, [%sp + 0x28]
	std	%i4, [%sp + 0x30]
	std	%i6, [%sp + 0x38]
	/* Leave saved window. */
	restore

.Lwodone:
	/*
	 * %l4: %g2 at trap time
	 * %l5: %g3 at trap time
	 *
	 * Save the state of the interrupted task including global registers on
	 * the task stack.
	 *
	 * IMPORTANT: Globals are saved here as well on the task stack, since a
	 * context switch might happen before the context of this interrupted
	 * task is restored.
	 */

	/* Allocate stack for isr context including ABI frame. */
	sub	%fp, ISF_SIZE, %sp
	/*
	 * %fp: %sp of interrupted task
	 * %sp: %sp of interrupted task - ISF_SIZE.
	 *      (fits a full ABI frame + what we store here)
	 *
	 * Save the interrupted context.
	 */
	std	%l0, [%sp + ISF_PSR_OFFSET]     /* psr pc */
	st	%l2, [%sp + ISF_NPC_OFFSET]     /* npc */
	st	%g1, [%sp + ISF_G1_OFFSET]      /* g1 */
	std	%l4, [%sp + ISF_G2_OFFSET]      /* g2  g3 */
	st	%g4, [%sp + ISF_G4_OFFSET]      /* g4 */
	std	%i0, [%sp + ISF_I0_OFFSET]      /* i0  i1 */
	std	%i2, [%sp + ISF_I2_OFFSET]      /* i2  i3 */
	std	%i4, [%sp + ISF_I4_OFFSET]      /* i4  i5 */
	std	%i6, [%sp + ISF_I6_OFFSET]      /* fp  i7 */
	rd	%y, %g1
	st	%g1, [%sp + ISF_Y_OFFSET]	/* y */

	/* %l5: reference to _kernel */
	set	_kernel, %l5
	/* Switch to interrupt stack. */
	mov	%sp, %fp
	ld	[%l5 + _kernel_offset_to_irq_stack], %sp

	/* Allocate a full C stack frame */
	sub	%sp, STACK_FRAME_SIZE, %sp
	/*
	 * %fp: %sp of interrupted task - ISF_SIZE.
	 * %sp: irq stack - 96. An ABI frame
	 */

	/* Enable traps, raise PIL to mask all maskable interrupts. */
	or	%l0, PSR_PIL, %o0
	wr	%o0, PSR_ET, %psr
	nop
	nop
	nop

#ifdef CONFIG_TRACING_ISR
	call	sys_trace_isr_enter
	 nop
#endif

	/* SPARC interrupt request level is the first argument */
	call	z_sparc_enter_irq
	 mov	%l3, %o0

#ifdef CONFIG_TRACING_ISR
	call	sys_trace_isr_exit
	 nop
#endif

	/*
	 * Switch back to interrupted task stack
	 * %fp: %sp of interrupted task - ISF_SIZE.
	 * %sp: irq stack - 96. An ABI frame
	 */
	mov	%fp, %sp
	/*
	 * %fp: %sp of interrupted task - ISF_SIZE.
	 * %sp: %sp of interrupted task - ISF_SIZE.
	 */

#ifdef CONFIG_PREEMPT_ENABLED
	sub	%sp, (96+8), %sp
	call	z_arch_get_next_switch_handle
	 add	%sp, 96, %o0
	/* we get old thread as "return value" on stack */
	ld	[%sp + 96], %o1
	/*
	 * o0: new thread
	 * o1: old thread
	 */
	cmp	%o0, %o1
	beq	.Lno_reschedule
	 nop
	call	z_sparc_context_switch
	 nop
.Lno_reschedule:
	add	%sp, (96+8), %sp
#endif /* CONFIG_PREEMPT_ENABLED */

	/* Restore the interrupted context. */
	ld	[%sp + ISF_Y_OFFSET], %g1
	wr	%g1, 0, %y

	ldd	[%sp + ISF_PSR_OFFSET], %l0     /* psr, pc */
	ld	[%sp + ISF_NPC_OFFSET], %l2     /* npc */
	/* NOTE: %g1 will be restored later */
	ldd	[%sp + ISF_G2_OFFSET], %g2
	ld	[%sp + ISF_G4_OFFSET], %g4
	ldd	[%sp + ISF_I0_OFFSET], %i0
	ldd	[%sp + ISF_I2_OFFSET], %i2
	ldd	[%sp + ISF_I4_OFFSET], %i4
	ldd	[%sp + ISF_I6_OFFSET], %i6
	/* %g1 is used to access the stack frame later */
	mov	%sp, %g1

	/*
	 * Install the PSR we got from the interrupt context. Current PSR.CWP
	 * is preserved. Keep PSR.ET=0 until we do "rett".
	 */
	rd	%psr, %l3
	and	%l3, PSR_CWP, %l3
	andn	%l0, (PSR_CWP | PSR_ET), %l0
	or	%l3, %l0, %l0
	mov	%l0, %psr
	nop
	nop
	nop

	/* Calculate %l6 := (cwp+1) % NWIN */
	rd	%wim, %l3
	set	(CONFIG_SPARC_NWIN), %l7
	add	%l0, 1, %l6
	and	%l6, PSR_CWP, %l6
	cmp	%l6, %l7
	bge,a	.Lwrapok
	 mov	0, %l6

.Lwrapok:
	/* Determine if we must prepare the return window. */
	/* %l5 := %wim >> (cwp+1) */
	srl	%l3, %l6, %l5
	/* %l5 is 1 if (cwp+1) is an invalid window */
	cmp	%l5, 1
	bne	.Lwudone
	 sub	%l7, 1, %l7             /* %l7 := NWIN - 1 */

	/* Do the window underflow. */
	sll	%l3, 1, %l4
	srl	%l3, %l7, %l5
	wr	%l4, %l5, %wim
	nop
	nop
	nop

	restore
	ldd	[%g1 + 0x00], %l0
	ldd	[%g1 + 0x08], %l2
	ldd	[%g1 + 0x10], %l4
	ldd	[%g1 + 0x18], %l6
	ldd	[%g1 + 0x20], %i0
	ldd	[%g1 + 0x28], %i2
	ldd	[%g1 + 0x30], %i4
	ldd	[%g1 + 0x38], %i6
	save

.Lwudone:
	/*
	 * Restore %psr since we may have trashed condition codes. PSR.ET is
	 * still 0.
	 */
	wr	%l0, %psr
	nop
	nop
	nop

	/* restore g1 */
	ld	[%g1 + ISF_G1_OFFSET], %g1

	jmp	%l1
	 rett	%l2
