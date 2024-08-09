/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <zephyr/toolchain.h>
#include <microblaze/microblaze_regs.h>
#include <microblaze/microblaze_asm.h>


.extern _interrupt_handler
.globl microblaze_emulate_isr

/* void microblaze_emulate_isr(void)
 * Must be called with irqs unlocked!
 * Must be called via inline asm with r14 as link register
 */
SECTION_FUNC(exception.other, microblaze_emulate_isr)
	.ent microblaze_emulate_isr
	/* Set the interrupt return address to after the
	 * call to microblaze_emulate_isr */
	ADD_IMM(r14, 4)
	brai _interrupt_handler
	.end    microblaze_emulate_isr
