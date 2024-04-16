/*
 * Copyright (c) 2018 Marvell
 * Copyright (c) 2018 Lexmark International, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Populated vector table in ROM
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

SECTION_SUBSEC_FUNC(exc_vector_table,_vector_table_section,_vector_table)
	ldr pc, =z_arm_reset             /*                   offset 0 */
	ldr pc, =z_arm_undef_instruction /* undef instruction offset 4 */
	ldr pc, =z_arm_svc               /* svc               offset 8 */
	ldr pc, =z_arm_prefetch_abort    /* prefetch abort offset  0xc */
	ldr pc, =z_arm_data_abort        /* data abort     offset 0x10 */
	nop				 /*                offset 0x14 */
#ifdef CONFIG_GEN_SW_ISR_TABLE
	ldr pc, =_isr_wrapper		 /* IRQ            offset 0x18 */
#else
	ldr pc, =z_irq_spurious
#endif
	ldr pc, =z_arm_nmi               /* FIQ            offset 0x1c */
