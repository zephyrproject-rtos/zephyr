/* sw_isr_table.S - ISR table for static ISR declarations for ARC */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

/*
 * enable preprocessor features, such
 * as %expr - evaluate the expression and use it as a string
 */
.altmacro

/*
 * Define an ISR table entry
 * Define symbol as weak and give the section .gnu.linkonce
 * prefix. This allows linker overload the symbol and the
 * whole section by the one defined by a device driver
 */
.macro _isr_table_entry_declare index
	WDATA(_isr_irq\index)
	.section .gnu.linkonce.isr_irq\index
	_isr_irq\index: .word 0xABAD1DEA, _irq_spurious
.endm

/*
 * Declare the ISR table
 */
.macro _isr_table_declare from, to
	counter = \from
	.rept (\to - \from)
		_isr_table_entry_declare %counter
		counter = counter + 1
	.endr
.endm

GTEXT(_irq_spurious)
GDATA(_sw_isr_table)

.section .isr_irq16
.align
_sw_isr_table:

/*In ARC architecture, IRQ 0-15 are reserved for the system and are not
assignable by the user, for that reason the isr table linker section
start at IRQ 16*/
_isr_table_declare 16 CONFIG_NUM_IRQS
