/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/riscv/csr.h>

GTEXT(__init)
GTEXT(__start)

GTEXT(_trap_vector_table)
GTEXT(_isr_wrapper)

	.align  1
	.option norvc;
SECTION_FUNC(init, __init)
	j	__start
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00000013
	.word 0x00100073

	.align  1
	.option norvc;
SECTION_FUNC(init, _trap_vector_table)
	j	__init
	j	0		/* reserved */
	j	_isr_wrapper	/* NMI */
	j	_isr_wrapper	/* Hard Fault */
	j	0		/* reserved */
	j	_isr_wrapper	/* Ecall M Mode */
	j	0		/* reserved */
	j	0		/* reserved */
	j	_isr_wrapper	/* Ecall U Mode */
	j	_isr_wrapper	/* Break Point */
	j	0		/* reserved */
	j	0		/* reserved */
	j	_isr_wrapper	/* SysTick */
	j	0		/* reserved */
	j	_isr_wrapper	/* SW */
	j	0		/* reserved */
