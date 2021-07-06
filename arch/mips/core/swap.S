/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <offsets_short.h>
#include <arch/cpu.h>

#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/cpu.h>

/*
 * unsigned int arch_swap(unsigned int key)
 *
 * Always called with interrupts locked
 * key is stored in a0 register
 */

LEAF(arch_swap)

	/* Make a system call to perform context switch */
	syscall

	/*
	 * when thread is rescheduled, unlock irq and return.
	 * Restored register v0 contains IRQ lock state of thread.
	 */
	la k0, _kernel

	/* Get pointer to _kernel.current */
	lw k1, _kernel_offset_to_current(k0)

	/* Load return value of arch_swap function in register v0 */
	lw v0, _thread_offset_to_swap_return_value(k1)

	/*
	 * Unlock irq, following IRQ lock state in v0 register.
	 */
	mfc0    k0, C0_SR
	or      k0, k0, a0
	mtc0    k0, C0_SR
	ehb

	/* Return */
	jr ra

END(arch_swap)
