/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.text
	.globl	microblaze_disable_exceptions
	.ent	microblaze_disable_exceptions
	.balign 4
microblaze_disable_exceptions:
#if CONFIG_MICROBLAZE_USE_MSR_INSTR == 1
	rtsd    r15, 8
	msrclr  r0, 0x100
#else
	mfs     r4, rmsr;
	/* Turn OFF the EE bit */
	andi    r4, r4, ~(0x100);
	mts     rmsr, r4;
	rtsd    r15, 8;
	nop;
#endif
.end microblaze_disable_exceptions
