/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.text
	.globl	microblaze_disable_dcache
	.ent	microblaze_disable_dcache
	.balign 4
microblaze_disable_dcache:
#if CONFIG_MICROBLAZE_USE_MSR_INSTR == 1
	rtsd	r15, 8
	msrclr r0, 0x80
#else /* CONFIG_MICROBLAZE_USE_MSR_INSTR == 1 */
	addik   r1, r1, -8
	mfs	r11, rmsr
	andi	r11, r11, ~(0x80)
	mts	rmsr, r11
	addik	r1, r1, 8
	rtsd	r15, 8
	nop
#endif /*CONFIG_MICROBLAZE_USE_MSR_INSTR == 1*/
	.end	microblaze_disable_dcache
