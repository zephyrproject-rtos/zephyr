/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.section .text.vectors
	.global reset_vector

/* The reset vector, jumps to startup code */
reset_vector:

	/* _vector_table: Only 256-byte aligned addresses are allowed */
	la      t0, _vector_table
	csrw    mtvec, t0

	j __start

__start:
	/* setup the stack pointer */
	la      sp, __stack_top

	/* Clear BSS section */
	la      t0, __bss_start
	la      t1, __bss_end
clear_bss:
	bge     t0, t1, done_bss
	sw      zero, 0(t0)
	addi    t0, t0, 4
	j       clear_bss
done_bss:

	call lp_core_startup
loop:
	j loop
