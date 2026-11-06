/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	ldr pc, =z_arm_reset                    /*                   offset 0 */
	ldr pc, =z_arm_undef_instruction        /* undef instruction offset 4 */
	ldr pc, =z_arm_swi                      /* swi               offset 8 */
	ldr pc, =z_arm_prefetch_abort           /* prefetch abort offset  0xc */
	ldr pc, =z_arm_data_abort               /* data abort     offset 0x10 */
	b .                                     /* reserved       offset 0x14 */
	ldr pc, =_isr_wrapper                   /* IRQ            offset 0x18 */
	ldr pc, =z_arm_fiq                      /* FIQ            offset 0x1c */
