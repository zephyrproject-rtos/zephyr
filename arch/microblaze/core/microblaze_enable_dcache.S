/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



	.text
	.globl	microblaze_enable_dcache
	.ent	microblaze_enable_dcache
	.balign 4
microblaze_enable_dcache:

#if CONFIG_MICROBLAZE_USE_MSR_INSTR == 1
	rtsd	r15, 8
	msrset  r0, 0x80
#else /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
	#Read the MSR register
	mfs	r8, rmsr
	#Set the interrupt enable bit
	ori	r8, r8, 0x80
	#Save the MSR register
	mts	rmsr, r8
	#Return
	rtsd	r15, 8
	nop
#endif /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
	.end	microblaze_enable_dcache
