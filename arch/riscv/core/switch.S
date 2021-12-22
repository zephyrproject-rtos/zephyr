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
GTEXT(arch_switch)
GTEXT(z_thread_entry_wrapper)

/* Use ABI name of registers for the sake of simplicity */

/*
 * void arch_switch(void *switch_to, void **switched_from);
 *
 * Always called with interrupts locked
 *
 * a0 = (struct k_thread *) switch_to
 * a1 = (struct k_thread **) address of output thread switch_handle field
 */
SECTION_FUNC(exception.other, arch_switch)

	/* Make a system call to perform context switch */
	ecall

	/* Return */
	ret


/*
 * void z_thread_entry_wrapper(k_thread_entry_t, void *, void *, void *)
 */
SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	/*
	 * z_thread_entry_wrapper is called for every new thread upon the return
	 * of arch_switch or ISR. Its address, as well as its input function
	 * arguments thread_entry_t, void *, void *, void * are restored from
	 * the thread stack (initialized via function _thread).
	 * In this case, thread_entry_t, * void *, void * and void * are stored
	 * in registers a0, a1, a2 and a3. These registers are used as arguments
	 * to function z_thread_entry. Hence, just call z_thread_entry with
	 * return address set to 0 to indicate a non-returning function call.
	 */

	jal x0, z_thread_entry
