/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* imports */
GTEXT(__reset)
GTEXT(__irq_wrapper)

/*
 * following riscv32-qemu specs
 * IVT is placed at 0x000001000 and is mapped as follows:
 * 0x00001000: reset
 * 0x00001004: non-maskable interrupt (nmi) vector
 * 0x00001010: machine trap (mt) vector
 *
 * Call __irq_wrapper to handle all interrupts/exceptions/faults
 */
SECTION_FUNC(vectors, vinit)
	.option norvc;

	/*
	 * jal instruction cannot be used to jump to address whose offset
	 * is > 12-bits wide. In this case, we have to use a call or tail
	 * instruction to jump to a far-away sub-routine.
	 *
	 * Given that IVT is found at a different address-space than the
	 * RAM in riscv32-qemu, we have to use call or tail instructions
	 * to jump to __reset or __isr_wrapper subroutines.
	 * However, call or tail instructions are pseudo instructions,
	 * which generate two base-instructions upon compilation. In this case,
	 * using them at a particular entry in the IVT will overwrite the next
	 * entry in the IVT. For example, using tail instruction in the
	 * reset vector, will overwrite the nmi-vector entry. To prevent this,
	 * perform a two-phase jump instructions to __reset or __irq_wrapper
	 * subroutines. The first jump performs a jal instruction, which will
	 * jump to an offset in the same vector address-space, but outside the
	 * IVT. The second jump performs a tail instruction to the __reset
	 * or __irq_wrapper subroutines.
	 */

	/* Call __reset for reset vector */
	jal x0, do_reset

	/* Call __irq_wrapper for nmi vector */
	jal x0, do_irq_wrapper

	.org 0x10
	/* Call __irq_wrapper for mt vector */
	jal x0, do_irq_wrapper

	.org 0x400 /* we are outside IVT */
do_reset:
	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to __irq_wrapper, so that we jump directly to __irq_wrapper,
	 * instead to the default machine trap vector address in IVT.
	 * This will preserve us from performing two jump instructions upon
	 * an interrupt.
	 */
	la t0, __irq_wrapper
	csrw mtvec, t0

	/* Jump to __reset */
	tail __reset

do_irq_wrapper:
	tail __irq_wrapper
