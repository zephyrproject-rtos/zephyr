/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
	MicroBlaze Vector Map for standalone executables

	Address        Vector type             Label
	-------        -----------             ------

	# 0x00 #        (-- IMM --)
	# 0x04 #        Reset                  _start

	# 0x08 #        (-- IMM --)
	# 0x0c #        Software Exception     _exception_handler

	# 0x10 #        (-- IMM --)
	# 0x14 #        Hardware Interrupt     _interrupt_handler

	# 0x18 #        (-- IMM --)
	# 0x1C #        Breakpoint Exception   (-- Don't Care --)

	# 0x20 #        (-- IMM --)
	# 0x24 #        Hardware Exception     _hw_exception_handler

*/

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

/* imports */
GTEXT(_PrepC)

	.section .vectors.sw_exception, "ax"
	.balign 4
_vector_sw_exception:
	brai     _exception_handler_entry

	.section .vectors.interrupt, "ax"
	.balign 4
_vector_interrupt:
	brai     _interrupt_handler

	.section .vectors.hw_exception, "ax"
	.balign 4
_vector_hw_exception:
	brai     _exception_handler_entry

	.section .text
	.globl _start
	.balign 4
	.ent _start
	.type _start, @function

_start:
	/* Set the Small Data Anchors and the stack pointer */
	ori    r13, r0, _SDA_BASE_
	ori    r2, r0, _SDA2_BASE_

	/* Initialize global pointer with the linker variable we set */
	ori    r20, r0, _gp

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	ori    r3, r0, z_interrupt_stacks
	addik  r4, r3, __z_interrupt_stack_SIZEOF
	ori    r5, r0, 0xaaaaaaaa

	/* Populate z_interrupt_stacks with 0xaaaaaaaa */
aa_loop:
	sw     r5, r0, r3
	addik  r3, r3, 4
	cmpu	 r6, r3, r4
	bnei   r6, aa_loop
#endif

	/* Load the initial stack */
	ori    r1, r0, z_interrupt_stacks
	addik  r1, r1, __z_interrupt_stack_SIZEOF

/* Initialize BSS and run program */
  /* clear SBSS */
	addi	r6, r0, __sbss_start
	addi	r7, r0, __sbss_end
	rsub	r18, r6, r7
	blei	r18, .Lendsbss

.Lloopsbss:
	swi	r0, r6, 0
	addi	r6, r6, 4
	rsub	r18, r6, r7
	bgti	r18, .Lloopsbss
.Lendsbss:

/* bss region is cleaned up by _PrepC */

	addi	r6, r0, 0              /* Initialize argc = 0  */
	addi	r7, r0, 0              /* Set envp = NULL*/
	brlid	r15, _PrepC            /* Execute the program */
	addi	r5, r0, 0              /* Set argv = NULL */

	/* Call exit with the return value of main */
	brlid  r15, _exit
	addik  r5, r3, 0

	/* Control does not reach here */
	.end _start
