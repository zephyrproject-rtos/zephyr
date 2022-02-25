/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <arch/riscv/syscall.h>
#include "asm_macros.inc"

/* exports */
GTEXT(arch_swap)

/* Use ABI name of registers for the sake of simplicity */

/*
 * unsigned int arch_swap(unsigned int key)
 *
 * Always called with interrupts locked
 * key is stored in a0 register
 */
SECTION_FUNC(exception.other, arch_swap)

	/* Make a system call to perform context switch */
	li a7, RV_ECALL_CONTEXT_SWITCH
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
	lr t1, _kernel_offset_to_current(t0)

	/* Load return value of arch_swap function in temp register t2 */
	lw t2, _thread_offset_to_swap_return_value(t1)

	/*
	 * Unlock irq, following IRQ lock state in a0 register.
	 * Use atomic instruction csrrs to do so.
	 */
	andi a0, a0, MSTATUS_IEN
	csrs mstatus, a0

	/* Set value of return register a0 to value of register t2 */
	mv a0, t2

	/* Return */
	ret
