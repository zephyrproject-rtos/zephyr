/* Copyright 2026 Espressif Systems (Shanghai) PTE LTD
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

/*
 * CLIC non-vectored exception entry.
 * mtvec (mode=3) points here for exceptions and non-vectored interrupts.
 * Must be aligned to 64 bytes (mtvec[31:6] << 6).
 */
	.global  _vector_table
	.section .exception_vectors.text
	.balign   0x40
	.type     _vector_table, @function

_vector_table:
	.option push
	.option norvc
	j _isr_wrapper
	.option pop

/*
 * CLIC hardware-vectored interrupt table.
 * mtvt CSR points here. Each entry is a function pointer (address).
 * On interrupt with SHV=1, the CLIC reads mtvt[4*int_id] and jumps there.
 * 48 entries cover all CLIC interrupt lines (0-15 internal, 16-47 external).
 */
	.balign 0x40
	.global _mtvt_table
	.type   _mtvt_table, @function
_mtvt_table:
	.rept (48)
	.word _isr_wrapper
	.endr
	.size _mtvt_table, .-_mtvt_table
