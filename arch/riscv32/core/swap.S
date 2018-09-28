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
GTEXT(_thread_entry_wrapper)

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
	addi sp, sp, -__NANO_ESF_SIZEOF

	sw ra, __NANO_ESF_ra_OFFSET(sp)
	sw gp, __NANO_ESF_gp_OFFSET(sp)
	sw tp, __NANO_ESF_tp_OFFSET(sp)
	sw t0, __NANO_ESF_t0_OFFSET(sp)
	sw t1, __NANO_ESF_t1_OFFSET(sp)
	sw t2, __NANO_ESF_t2_OFFSET(sp)
	sw t3, __NANO_ESF_t3_OFFSET(sp)
	sw t4, __NANO_ESF_t4_OFFSET(sp)
	sw t5, __NANO_ESF_t5_OFFSET(sp)
	sw t6, __NANO_ESF_t6_OFFSET(sp)
	sw a0, __NANO_ESF_a0_OFFSET(sp)
	sw a1, __NANO_ESF_a1_OFFSET(sp)
	sw a2, __NANO_ESF_a2_OFFSET(sp)
	sw a3, __NANO_ESF_a3_OFFSET(sp)
	sw a4, __NANO_ESF_a4_OFFSET(sp)
	sw a5, __NANO_ESF_a5_OFFSET(sp)
	sw a6, __NANO_ESF_a6_OFFSET(sp)
	sw a7, __NANO_ESF_a7_OFFSET(sp)

	call read_timer_start_of_swap

	lw ra, __NANO_ESF_ra_OFFSET(sp)
	lw gp, __NANO_ESF_gp_OFFSET(sp)
	lw tp, __NANO_ESF_tp_OFFSET(sp)
	lw t0, __NANO_ESF_t0_OFFSET(sp)
	lw t1, __NANO_ESF_t1_OFFSET(sp)
	lw t2, __NANO_ESF_t2_OFFSET(sp)
	lw t3, __NANO_ESF_t3_OFFSET(sp)
	lw t4, __NANO_ESF_t4_OFFSET(sp)
	lw t5, __NANO_ESF_t5_OFFSET(sp)
	lw t6, __NANO_ESF_t6_OFFSET(sp)
	lw a0, __NANO_ESF_a0_OFFSET(sp)
	lw a1, __NANO_ESF_a1_OFFSET(sp)
	lw a2, __NANO_ESF_a2_OFFSET(sp)
	lw a3, __NANO_ESF_a3_OFFSET(sp)
	lw a4, __NANO_ESF_a4_OFFSET(sp)
	lw a5, __NANO_ESF_a5_OFFSET(sp)
	lw a6, __NANO_ESF_a6_OFFSET(sp)
	lw a7, __NANO_ESF_a7_OFFSET(sp)

	/* Release stack space */
	addi sp, sp, __NANO_ESF_SIZEOF
#endif
	ecall

	/*
	 * when thread is rescheduled, unlock irq and return.
	 * Restored register a0 contains IRQ lock state of thread.
	 *
	 * Prior to unlocking irq, load return value of
	 * __swap to temp register t2 (from
	 * _thread_offset_to_swap_return_value). Normally, it should be -EAGAIN,
	 * unless someone has previously called _set_thread_return_value(..).
	 */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	lw t1, _kernel_offset_to_current(t0)

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
 * void _thread_entry_wrapper(k_thread_entry_t, void *, void *, void *)
 */
SECTION_FUNC(TEXT, _thread_entry_wrapper)
	/*
	 * _thread_entry_wrapper is called for every new thread upon the return
	 * of __swap or ISR. Its address, as well as its input function
	 * arguments thread_entry_t, void *, void *, void * are restored from
	 * the thread stack (initialized via function _thread).
	 * In this case, thread_entry_t, * void *, void * and void * are stored
	 * in registers a0, a1, a2 and a3. These registers are used as arguments
	 * to function _thread_entry. Hence, just call _thread_entry with
	 * return address set to 0 to indicate a non-returning function call.
	 */

	jal x0, _thread_entry
