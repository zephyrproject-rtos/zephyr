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
