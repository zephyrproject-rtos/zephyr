/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <kernel_structs.h>
#include <offsets_short.h>

#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/cpu.h>

GTEXT(_k_neg_eagain)

LEAF(__isr_vec)
	.set	push
	.set	noat
	la	k0, _mips_interrupt
	jr	k0
	.set	pop
END(__isr_vec)

#define OS_STACK_ALIGNMENT (8)

#define CTX_ALIGNED_SIZE ((((CTX_SIZE - 1) / OS_STACK_ALIGNMENT) + 1) * \
	OS_STACK_ALIGNMENT)

LEAF(_mips_interrupt)
	.set    noat
	/* Context save */
	addi    sp, sp, -CTX_ALIGNED_SIZE
	REG_S   $1, CTX_REG(1)(sp)
	REG_S   $2, CTX_REG(2)(sp)
	REG_S   $3, CTX_REG(3)(sp)
	REG_S   $4, CTX_REG(4)(sp)
	REG_S   $5, CTX_REG(5)(sp)
	REG_S   $6, CTX_REG(6)(sp)
	REG_S   $7, CTX_REG(7)(sp)
	REG_S   $8, CTX_REG(8)(sp)
	REG_S   $9, CTX_REG(9)(sp)
	REG_S   $10, CTX_REG(10)(sp)
	REG_S   $11, CTX_REG(11)(sp)
	REG_S   $12, CTX_REG(12)(sp)
	REG_S   $13, CTX_REG(13)(sp)
	REG_S   $14, CTX_REG(14)(sp)
	REG_S   $15, CTX_REG(15)(sp)
	REG_S   $16, CTX_REG(16)(sp)
	REG_S   $17, CTX_REG(17)(sp)
	REG_S   $18, CTX_REG(18)(sp)
	REG_S   $19, CTX_REG(19)(sp)
	REG_S   $20, CTX_REG(20)(sp)
	REG_S   $21, CTX_REG(21)(sp)
	REG_S   $22, CTX_REG(22)(sp)
	REG_S   $23, CTX_REG(23)(sp)
	REG_S   $24, CTX_REG(24)(sp)
	REG_S   $25, CTX_REG(25)(sp)
	REG_S   $26, CTX_REG(26)(sp)
	REG_S   $27, CTX_REG(27)(sp)
	REG_S   $28, CTX_REG(28)(sp)
	REG_S   $29, CTX_REG(29)(sp)
	REG_S   $30, CTX_REG(30)(sp)
	REG_S   $31, CTX_REG(31)(sp)
	PTR_S   $0, CTX_LINK(sp)        # Clear the link field

	#if (__mips_isa_rev < 6)
	mfhi	t0
	mflo	t1
	REG_S	t0, CTX_HI0(sp)
	REG_S	t1, CTX_LO0(sp)
	#endif

	# cp0
	PTR_MFC0 t0, C0_EPC
	REG_S	t0, CTX_EPC(sp)
	PTR_MFC0 t1, C0_BADVADDR
	REG_S	t1, CTX_BADVADDR(sp)
	mfc0	t0, C0_SR
	sw	t0, CTX_STATUS(sp)
	mfc0	t1, C0_CR
	sw	t1, CTX_CAUSE(sp)

	/* FIXME */
	li k0, 0x7c /* EXCMASK */
	and k1, k0, t1
	li k0, 0x20 /* SYSCALL MASK, 8 << 2 */
	and k1, k0, k1
	beq k0, k1, is_kernel_syscall

	/* status is in $8 (t0), cause is in $9 (t1) */
	/* mask out the interrupts not enabled in status */
	and	t1, t1, t0
	li	a0, CR_IP_MASK
	and	a0, a0, t1
	srl	a0, a0, CR_IP_SHIFT
	/* check if interrupt */
	bnez	a0, interrupt

	move	a0, sp

/*
 * FIXME
 *
 * We can't just eret after z_irq_spurious().
 * Here is RISC-V tip:
 *
 *  la ra, no_reschedule
 *  tail _Fault
 *
 */

	/* FIXME: z_irq_spurious is a inappropriate name */
	/* spurious interrupt */
	jal	z_irq_spurious
	eret

is_kernel_syscall:

	/*
         * EPC contains the virtual address of the 'syscall' instruction.
         * Increment saved EPC by 4.
         */
	REG_L   k0, CTX_EPC(sp)
	addi    k0, k0, 4
	REG_S	k0, CTX_EPC(sp)

	/*
	 * Go to reschedule to handle context-switch
	 */
	j reschedule


interrupt:
	.set    at
	la k1, _kernel

	/* save the thread stack pointer */
	move k0, sp

	/* Switch to interrupt stack */
	lw sp, _kernel_offset_to_irq_stack(k1)

	/* save thread stack pointer on the interrupt stack */
	addi sp, sp, -16
	sw k0, 0(sp)

	/* call c function to handle entering the irq */
	jal z_mips_enter_irq

	/* Restore thread stack pointer */
	lw sp, 0(sp)

#ifdef CONFIG_PREEMPT_ENABLED
	/* Get pointer to _kernel.current */
	lw t2, _kernel_offset_to_current(k1)

	/*
	 * If non-preemptible thread, do not schedule
	 * (see explanation of preempt field in kernel_structs.h
	 */
	li t4, _NON_PREEMPT_THRESHOLD

	/*
	 * Check if next thread to schedule is current thread.
	 * If yes do not perform a reschedule
	 */
	lw t3, _kernel_offset_to_ready_q_cache(k1)
	beq t3, t2, no_reschedule
#else
	j no_reschedule
#endif /* CONFIG_PREEMPT_ENABLED */

reschedule:
	/* Get reference to _kernel */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	lw t1, _kernel_offset_to_current(t0)

	/*
	 * Save stack pointer of current thread and set the default return value
	 * of _Swap to _k_neg_eagain for the thread.
	 */
	sw sp, _thread_offset_to_sp(t1)
	la t2, _k_neg_eagain
	lw t3, 0x00(t2)
	sw t3, _thread_offset_to_swap_return_value(t1)

	/* Get next thread to schedule. */
	lw t1, _kernel_offset_to_ready_q_cache(t0)

	/*
	 * Set _kernel.current to new thread loaded in t1
	 */
	sw t1, _kernel_offset_to_current(t0)

	/* Switch to new thread stack */
	lw sp, _thread_offset_to_sp(t1)

no_reschedule:
	.set    noat

	mfc0    t1, C0_SR
	li      t2, SR_IE
	not     t2, t2
	and     t1, t1, t2
	mtc0    t1, C0_SR
	ehb

	/* restore thread context */
#if (__mips_isa_rev < 6)
	REG_L	t1, CTX_HI0(sp)
	REG_L	t2, CTX_LO0(sp)
	mthi	t1
	mtlo	t2
#endif

	REG_L   $1, CTX_REG(1)(sp)
	REG_L   $2, CTX_REG(2)(sp)
	REG_L   $3, CTX_REG(3)(sp)
	REG_L   $4, CTX_REG(4)(sp)
	REG_L   $5, CTX_REG(5)(sp)
	REG_L   $6, CTX_REG(6)(sp)
	REG_L   $7, CTX_REG(7)(sp)
	REG_L   $8, CTX_REG(8)(sp)
	REG_L   $9, CTX_REG(9)(sp)
	REG_L   $10, CTX_REG(10)(sp)
	REG_L   $11, CTX_REG(11)(sp)
	REG_L   $12, CTX_REG(12)(sp)
	REG_L   $13, CTX_REG(13)(sp)
	REG_L   $14, CTX_REG(14)(sp)
	REG_L   $15, CTX_REG(15)(sp)
	REG_L   $16, CTX_REG(16)(sp)
	REG_L   $17, CTX_REG(17)(sp)
	REG_L   $18, CTX_REG(18)(sp)
	REG_L   $19, CTX_REG(19)(sp)
	REG_L   $20, CTX_REG(20)(sp)
	REG_L   $21, CTX_REG(21)(sp)
	REG_L   $22, CTX_REG(22)(sp)
	REG_L   $23, CTX_REG(23)(sp)
	REG_L   $24, CTX_REG(24)(sp)
	REG_L   $25, CTX_REG(25)(sp)
	REG_L   $26, CTX_REG(26)(sp)
	REG_L   $27, CTX_REG(27)(sp)
	REG_L   $28, CTX_REG(28)(sp)
	/* sp restored last */
	REG_L   $30, CTX_REG(30)(sp)
	REG_L   $31, CTX_REG(31)(sp)

	REG_L   k0, CTX_EPC(sp)
	PTR_MTC0 k0, C0_EPC
	lw      k0, CTX_STATUS(sp)
	/* restore sp */
	REG_L   $29, CTX_REG(29)(sp)
	addi    sp, sp, CTX_ALIGNED_SIZE

	# STATUS here will have EXL set
	mtc0    k0, C0_SR
	ehb

	eret

END(_mips_interrupt)
