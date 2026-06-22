/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.section	.init.vector,"ax"

	.global _vector_table
	.type _vector_table, @function
_vector_table:
	.option push
	.option norvc

	/*
	 * Entry 0: LP core boots from 0x50000000 (first entry of vector table).
	 * Jump to reset_vector at 0x80 offset to begin execution.
	 */
	j reset_vector

	.rept 29
	j _panic_handler
	.endr
	j _interrupt_handler
	j _panic_handler

	.option pop
	.size _vector_table, .-_vector_table
