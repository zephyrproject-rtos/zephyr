/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <kernel_structs.h>
#include <offsets_short.h>

/* exports */
GTEXT(__swap)
GTEXT(z_thread_entry_wrapper)

/* Use ABI name of registers for the sake of simplicity */

/*
 * unsigned int __swap(unsigned int key)
 *
 * Always called with interrupts locked
 * key is stored in a0 register
 */
SECTION_FUNC(exception.other, __swap)

	/* Make a system call to perform context switch */
#ifdef CONFIG_EXECUTION_BENCHMARKING
	addi sp, sp, -__z_arch_esf_t_SIZEOF

	SR ra, __z_arch_esf_t_ra_OFFSET(sp)
	SR gp, __z_arch_esf_t_gp_OFFSET(sp)
	SR tp, __z_arch_esf_t_tp_OFFSET(sp)
	SR t0, __z_arch_esf_t_t0_OFFSET(sp)
	SR t1, __z_arch_esf_t_t1_OFFSET(sp)
	SR t2, __z_arch_esf_t_t2_OFFSET(sp)
	SR t3, __z_arch_esf_t_t3_OFFSET(sp)
	SR t4, __z_arch_esf_t_t4_OFFSET(sp)
	SR t5, __z_arch_esf_t_t5_OFFSET(sp)
	SR t6, __z_arch_esf_t_t6_OFFSET(sp)
	SR a0, __z_arch_esf_t_a0_OFFSET(sp)
	SR a1, __z_arch_esf_t_a1_OFFSET(sp)
	SR a2, __z_arch_esf_t_a2_OFFSET(sp)
	SR a3, __z_arch_esf_t_a3_OFFSET(sp)
	SR a4, __z_arch_esf_t_a4_OFFSET(sp)
	SR a5, __z_arch_esf_t_a5_OFFSET(sp)
	SR a6, __z_arch_esf_t_a6_OFFSET(sp)
	SR a7, __z_arch_esf_t_a7_OFFSET(sp)

	call read_timer_start_of_swap

	LR ra, __z_arch_esf_t_ra_OFFSET(sp)
	LR gp, __z_arch_esf_t_gp_OFFSET(sp)
	LR tp, __z_arch_esf_t_tp_OFFSET(sp)
	LR t0, __z_arch_esf_t_t0_OFFSET(sp)
	LR t1, __z_arch_esf_t_t1_OFFSET(sp)
	LR t2, __z_arch_esf_t_t2_OFFSET(sp)
	LR t3, __z_arch_esf_t_t3_OFFSET(sp)
	LR t4, __z_arch_esf_t_t4_OFFSET(sp)
	LR t5, __z_arch_esf_t_t5_OFFSET(sp)
	LR t6, __z_arch_esf_t_t6_OFFSET(sp)
	LR a0, __z_arch_esf_t_a0_OFFSET(sp)
	LR a1, __z_arch_esf_t_a1_OFFSET(sp)
	LR a2, __z_arch_esf_t_a2_OFFSET(sp)
	LR a3, __z_arch_esf_t_a3_OFFSET(sp)
	LR a4, __z_arch_esf_t_a4_OFFSET(sp)
	LR a5, __z_arch_esf_t_a5_OFFSET(sp)
	LR a6, __z_arch_esf_t_a6_OFFSET(sp)
	LR a7, __z_arch_esf_t_a7_OFFSET(sp)

	/* Release stack space */
	addi sp, sp, __z_arch_esf_t_SIZEOF
#endif
	ecall

	/*
	 * when thread is rescheduled, unlock irq and return.
	 * Restored register a0 contains IRQ lock state of thread.
	 *
	 * Prior to unlocking irq, load return value of
	 * __swap to temp register t2 (from
	 * _thread_offset_to_swap_return_value). Normally, it should be -EAGAIN,
	 * unless someone has previously called z_set_thread_return_value(..).
	 */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	LR t1, _kernel_offset_to_current(t0)

	/* Load return value of __swap function in temp register t2 */
	lw t2, _thread_offset_to_swap_return_value(t1)

	/*
	 * Unlock irq, following IRQ lock state in a0 register.
	 * Use atomic instruction csrrs to do so.
	 */
	andi a0, a0, SOC_MSTATUS_IEN
	csrrs t0, mstatus, a0

	/* Set value of return register a0 to value of register t2 */
	addi a0, t2, 0

	/* Return */
	jalr x0, ra


/*
 * void z_thread_entry_wrapper(k_thread_entry_t, void *, void *, void *)
 */
SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	/*
	 * z_thread_entry_wrapper is called for every new thread upon the return
	 * of __swap or ISR. Its address, as well as its input function
	 * arguments thread_entry_t, void *, void *, void * are restored from
	 * the thread stack (initialized via function _thread).
	 * In this case, thread_entry_t, * void *, void * and void * are stored
	 * in registers a0, a1, a2 and a3. These registers are used as arguments
	 * to function z_thread_entry. Hence, just call z_thread_entry with
	 * return address set to 0 to indicate a non-returning function call.
	 */

	jal x0, z_thread_entry
