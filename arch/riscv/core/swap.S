/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>

/* exports */
GTEXT(arch_swap)
GTEXT(z_thread_entry_wrapper)

/* Use ABI name of registers for the sake of simplicity */

/*
 * unsigned int arch_swap(unsigned int key)
 *
 * Always called with interrupts locked
 * key is stored in a0 register
 */
SECTION_FUNC(exception.other, arch_swap)

	/* Make a system call to perform context switch */
	ecall

	/*
	 * when thread is rescheduled, unlock irq and return.
	 * Restored register a0 contains IRQ lock state of thread.
	 *
	 * Prior to unlocking irq, load return value of
	 * arch_swap to temp register t2 (from
	 * _thread_offset_to_swap_return_value). Normally, it should be -EAGAIN,
	 * unless someone has previously called arch_thread_return_value_set(..).
	 */
	la t0, _kernel

	/* Get pointer to _kernel.current */
	RV_OP_LOADREG t1, _kernel_offset_to_current(t0)

	/* Load return value of arch_swap function in temp register t2 */
	lw t2, _thread_offset_to_swap_return_value(t1)

	/*
	 * Unlock irq, following IRQ lock state in a0 register.
	 * Use atomic instruction csrrs to do so.
	 */
	andi a0, a0, MSTATUS_IEN
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
	 * of arch_swap or ISR. Its address, as well as its input function
	 * arguments thread_entry_t, void *, void *, void * are restored from
	 * the thread stack (initialized via function _thread).
	 * In this case, thread_entry_t, * void *, void * and void * are stored
	 * in registers a0, a1, a2 and a3. These registers are used as arguments
	 * to function z_thread_entry. Hence, just call z_thread_entry with
	 * return address set to 0 to indicate a non-returning function call.
	 */

	jal x0, z_thread_entry
