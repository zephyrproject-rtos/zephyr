/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

.macro vector target
	.balign 16
	jr	\target
.endm

GDATA(z_rh850_vectors)

	.section .vectors, "ax"
z_rh850_vectors:
	vector	__start			/* 000H RESET */
	vector	z_rh850_exc_fe		/* 010H SYSERR */
	vector	z_rh850_exc_fe		/* 020H reserved */
	vector	z_rh850_exc_fe		/* 030H FETRAP */
	vector	z_rh850_trap0		/* 040H TRAP0 */
	vector	z_rh850_exc_ei		/* 050H TRAP1 */
	vector	z_rh850_exc_fe		/* 060H RIE */
	vector	z_rh850_exc_ei		/* 070H FPE, FPI */
#ifdef CONFIG_FXU
	vector	z_rh850_exc_ucpop	/* 080H UCPOP */
#else
	vector	z_rh850_exc_fe		/* 080H UCPOP */
#endif
	vector	z_rh850_exc_fe		/* 090H MIP, MDP */
	vector	z_rh850_exc_fe		/* 0A0H PIE */
	vector	z_rh850_exc_fe		/* 0B0H reserved */
	vector	z_rh850_exc_fe		/* 0C0H MAE */
	vector	z_rh850_exc_fe		/* 0D0H reserved */
	vector	z_rh850_exc_fe		/* 0E0H FENMI */
	vector	z_rh850_exc_fe		/* 0F0H FEINT */
	.rept	16
	vector	z_rh850_isr_wrapper	/* 100H-1F0H EIINT priority 0-15, 1F0H also 16-63 */
	.endr
