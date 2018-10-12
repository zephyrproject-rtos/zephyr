/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <offsets_short.h>

_ASM_FILE_PROLOGUE

GDATA(_interrupt_stack)

GTEXT(_do_software_reboot)
SECTION_FUNC(TEXT,_do_software_reboot)

	eors r0, r0

	/* move exception table back to 0 */
	ldr r1, =0xe000e000
	str r0, [r1, #0xd08] /* VTOR */

	ldr r0, [r0, #4]
	bx r0


GTEXT(_force_exit_one_nested_irq)
SECTION_FUNC(TEXT,_force_exit_one_nested_irq)

	ldr r0, =_SCS_ICSR_RETTOBASE
	ldr r1, =_SCS_ICSR
	ldr r1, [r1]
	ands.w r0, r1

	/*
	* If Z flag is set, we are nested, so un-nest one level and get
	* back to this function to unwind the next level; else, exit the
	* last interrupt by jumping to reboot code.
	*/
	ittee eq
		ldreq lr, =0xfffffff1
		ldreq r2, =_force_exit_one_nested_irq
		ldrne lr, =0xfffffffd
		ldrne r2, =_do_software_reboot

	ldr ip, =_interrupt_stack
	add.w ip, #(___esf_t_SIZEOF * 2) /* enough for a stack frame */
	ldr r1, =0xfffffffe
	and.w r2, r1
	str r2, [ip, #(6 * 4)]
	ldr r2, =0x01000000
	str r2, [ip, #(7 * 4)]

	ite eq
		moveq sp, ip
		msrne PSP, ip

	bx lr
