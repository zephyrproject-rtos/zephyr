/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/config/system.h>
#include <xtensa/hal.h>
#include <xtensa-asm2-context.h>

#include <offsets.h>

	.section    .iram1, "ax"
	.align      4
	.global     z_xtensa_backtrace_get_start
	.type       z_xtensa_backtrace_get_start, @function
z_xtensa_backtrace_get_start:
	entry   a1, 32
	/* Spill registers onto stack (excluding this function) */
	call8   xthal_window_spill
	/* a2, a3, a4 should be out arguments for i PC, i SP, i-1 PC respectively.
	* Use a6 and a7 as scratch */

	/* Load address for interrupted stack */
	l32i    a6, a5, 0
	/* Load i PC in a7 */
	l32i    a7, a6, ___xtensa_irq_bsa_t_pc_OFFSET
	/* Store value of i PC in a2 */
	s32i    a7, a2, 0
	/* Load value for (i-1) PC, which return address of i into a7 */
	l32i    a7, a6, ___xtensa_irq_bsa_t_a0_OFFSET
	/* Store value of (i-1) PC in a4 */
	s32i    a7, a4, 0
	/* Add base stack frame size in interrupted stack to get i SP */
	addi    a6, a6, ___xtensa_irq_bsa_t_SIZEOF
	/* Store i SP in a3 */
	s32i    a6, a3, 0
	retw
