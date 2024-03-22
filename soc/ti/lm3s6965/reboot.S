/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>

_ASM_FILE_PROLOGUE

GDATA(z_interrupt_stacks)

GTEXT(z_do_software_reboot)
SECTION_FUNC(TEXT,z_do_software_reboot)

	eors r0, r0

	/* move exception table back to 0 */
	ldr r1, =0xe000e000
	str r0, [r1, #0xd08] /* VTOR */

	ldr r0, [r0, #4]
	bx r0


GTEXT(z_force_exit_one_nested_irq)
SECTION_FUNC(TEXT,z_force_exit_one_nested_irq)

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
		ldreq r2, =z_force_exit_one_nested_irq
		ldrne lr, =0xfffffffd
		ldrne r2, =z_do_software_reboot

	ldr ip, =z_interrupt_stacks
	add.w ip, ip, #(___esf_t_SIZEOF * 2) /* enough for a stack frame */
	ldr r1, =0xfffffffe
	and.w r2, r1
	str r2, [ip, #(6 * 4)]
	ldr r2, =0x01000000
	str r2, [ip, #(7 * 4)]

	ite eq
		moveq sp, ip
		msrne PSP, ip

	bx lr
