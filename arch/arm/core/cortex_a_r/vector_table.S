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
#include "offsets_short.h"
#include "macro_priv.inc"

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
#ifndef CONFIG_USE_SWITCH
	ldr pc, =z_arm_nmi               /* FIQ            offset 0x1c */
#else
	ldr pc,=z_irq_spurious
#endif


#ifdef CONFIG_USE_SWITCH
GTEXT(z_arm_cortex_ar_exit_exc)
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_cortex_ar_exit_exc)

	/* Note:
	 * This function is expected to be *always* called with
	 * processor mode set to MODE_SYS.
	 */

	/* decrement exception depth */
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	sub r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

	/*
	 * Restore r0-r3, r12, lr, lr_und and spsr_und from the exception stack
	 * and return to the current thread.
	 */
	pop {r0-r3, r12, lr}
	rfeia sp!
#endif
