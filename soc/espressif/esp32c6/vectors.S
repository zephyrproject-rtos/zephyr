/* Copyright 2023 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc/soc.h"
#include "soc/interrupt_reg.h"
#include "riscv/rvruntime-frames.h"
#include "soc/soc_caps.h"
#include <zephyr/toolchain.h>

/* Imports */
GTEXT(_isr_wrapper)

	/* This is the vector table. MTVEC points here.
	 *
	 * Use 4-byte intructions here. 1 instruction = 1 entry of the table.
	 * The CPU jumps to MTVEC (i.e. the first entry) in case of an exception,
	 * and (MTVEC & 0xfffffffc) + (mcause & 0x7fffffff) * 4, in case of an interrupt.
	 *
	 * Note: for our CPU, we need to place this on a 256-byte boundary, as CPU
	 * only uses the 24 MSBs of the MTVEC, i.e. (MTVEC & 0xffffff00).
	 */

	.global  _esp32c6_vector_table
	.section .exception_vectors.text
	.balign   0x100
	.type     _esp32c6_vector_table, @function

_esp32c6_vector_table:
	.option push
	.option norvc
	.rept (32)
	j _isr_wrapper		/* 32 identical entries, all pointing to the interrupt handler */
	.endr
