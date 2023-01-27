/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.text
	.globl	microblaze_enable_exceptions
	.ent	microblaze_enable_exceptions
	.balign 4
microblaze_enable_exceptions:
#if CONFIG_MICROBLAZE_USE_MSR_INSTR == 1
	rtsd    r15, 8;
	msrset  r0, 0x100
#else /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
	mfs     r4, rmsr;
	/* Turn ON the EE bit */
	ori     r4, r4, 0x100;
	mts     rmsr, r4;
	rtsd    r15, 8;
	nop;
#endif /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
.end microblaze_enable_exceptions
