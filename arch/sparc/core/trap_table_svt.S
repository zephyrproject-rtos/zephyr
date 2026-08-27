/*
 * Copyright (c) 2023 Frontgrade Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file contains the trap entry for SPARC operating with
 * single-vector trap model, defined in SPARC V8E. The processor
 * redirects execution to a single entry on any trap event. From
 * there, two levels of look-up tables are used to find the trap
 * handler.
 *
 * - Execution time is constant.
 * - Condition flags are not modified.
 * - Provides handler with PSR in l0, TBR in l6
 * - This SVT implementation is less than 400 bytes long. (An MVT
 *   table is always 4096 bytes long.)
 *
 * See trap_table_mvt.S for information about SPARC trap types.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/sparc/sparc.h>

#ifdef CONFIG_IRQ_OFFLOAD
 #define IRQ_OFFLOAD_HANDLER    __sparc_trap_irq_offload
#else
 #define IRQ_OFFLOAD_HANDLER    __sparc_trap_fault
#endif

GTEXT(__sparc_trap_table)
GTEXT(__start)

SECTION_SUBSEC_FUNC(TEXT, traptable, __sparc_trap_table)
__start:
	rd	%psr, %l0
	mov	%tbr, %l6

	and	%l6, 0xf00, %l7
	srl	%l7, 6, %l7
	set	__sparc_trap_table_svt_level0, %l4
	ld	[%l4 + %l7], %l4

	and	%l6, 0x0f0, %l7
	srl	%l7, 2, %l7
	ld	[%l4 + %l7], %l4

	srl	%l6, 4, %l3
	jmp	%l4
	 and	%l3, 0xf, %l3 /* Interrupt level */

__sparc_trap_svt_in_trap:
	ta	0x00
	nop

SECTION_VAR(RODATA, __sparc_trap_table_svt_tables)
	.align	4
__sparc_trap_table_svt_level0:
	.word	__sparc_trap_table_svt_00
	.word	__sparc_trap_table_svt_10
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_80
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad
	.word	__sparc_trap_table_svt_allbad

__sparc_trap_table_svt_00:
	.word	__sparc_trap_reset
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_window_overflow
	.word	__sparc_trap_window_underflow
__sparc_trap_table_svt_allbad:
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault

__sparc_trap_table_svt_10:
	.word	__sparc_trap_fault
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt
	.word	__sparc_trap_interrupt

__sparc_trap_table_svt_80:
	.word	__sparc_trap_svt_in_trap
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_flush_windows
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_sw_set_pil
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	__sparc_trap_fault
	.word	IRQ_OFFLOAD_HANDLER
	.word	__sparc_trap_fault
	.word	__sparc_trap_except_reason
