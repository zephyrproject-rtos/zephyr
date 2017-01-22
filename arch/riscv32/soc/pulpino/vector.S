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
 * following pulpino datasheet, addr 0x00000000 - 0x00000058 are not used
 * in IVT. Hence, set them to nop.
 *
 * Call __irq_wrapper to handle all interrupts/exceptions/faults/ECALL
 *
 * ECALL is used to handle context switching of threads, as well as
 * IRQ offloading (when enabled).
 *
 * Interrupt Line 23: I2C IRQ 0x0000005C
 * Interrupt Line 24: UART IRQ 0x00000060
 * Interrupt Line 25: GPIO IRQ 0x00000064
 * Interrupt Line 26: SPI Master 0 0x00000068
 * Interrupt Line 27: SPI Master 1 0x0000006C
 * Interrupt Line 28: Timer A Overflow 0x00000070
 * Interrupt Line 29: Timer A Output Cmp 0x00000074
 * Interrupt Line 30: Timer B Overflow 0x00000078
 * Interrupt Line 31: Timer B Output Cmp 0x0000007C
 *
 * RESET 0x00000080 - call __reset

 * Illegal Instruction Exception 0x00000084
 * ECALL Instruction 0x00000088
 * Invalid Memory Access 0x0000008C
 */
SECTION_FUNC(vectors, vinit)
	.option norvc;

	/* nop addr 0x00000000 - 0x00000058 */
	.org 0x00
	.rept 23
	nop
	.endr

	/* Call __irq_wrapper for all interrupts */
	.org 0x5C
	.rept 9
	jal x0, __irq_wrapper
	.endr

	/* Call __reset for reset vector */
	.org 0x80
	jal x0, __reset

	/* Illegal instruction */
	jal x0, __irq_wrapper

	/* ECALL (system call) */
	jal x0, __irq_wrapper

	/* Invalid memory access */
	jal x0, __irq_wrapper
