/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#define _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#include <offsets.h>
#include "vector_table.h"

_ASM_FILE_PROLOGUE

GTEXT(__reset)

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, the main stack pointer (MSP) is already
 * pointing to a valid area in SRAM.
 *
 * Locking interrupts prevents anything but NMIs and hard faults from
 * interrupting the CPU. A default NMI handler is already in place in the
 * vector table, and the boot code should not generate hard fault, or we're in
 * deep trouble.
 *
 * We want to use the process stack pointer (PSP) instead of the MSP, since the
 * MSP is to be set up to point to the one-and-only interrupt stack during later
 * boot. That would not be possible if in use for running C code.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 *
 * @return N/A
 */

SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset)

/*
 * In non-XIP kernels, the entry point is located at the __reset symbol, which
 * is fetched by a XIP image playing the role of a bootloader, which jumps to
 * it, not through the reset vector mechanism. Such bootloaders might want to
 * search for a __start symbol instead, so create that alias here.
 */
#if !defined(CONFIG_XIP)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)
#endif

    /* lock interrupts: will get unlocked when switch to main task */
#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    cpsid i
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    movs.n r0, #_EXC_IRQ_DEFAULT_PRIO
    msr BASEPRI, r0
#endif

    /*
     * Set PSP and use it to boot without using MSP, so that it
     * gets set to _interrupt_stack during nanoInit().
     */
    ldr r0, =__CORTEXM_BOOT_PSP
    msr PSP, r0
    movs.n r0, #2	/* switch to using PSP (bit1 of CONTROL reg) */
    msr CONTROL, r0

#ifdef CONFIG_WDOG_INIT
    /* board-specific watchdog initialization is necessary */
    bl _WdogInit
#endif

    b _PrepC

#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)

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
	 *  If Z flag is set, we are nested, so un-nest one level and get back to
	 *  this function to unwind the next level; else, exit the last interrupt
	 *  by jumping to reboot code.
	 */
	ittee eq
		ldreq lr, =0xfffffff1
		ldreq r2, =_force_exit_one_nested_irq
		ldrne lr, =0xfffffffd
		ldrne r2, =_do_software_reboot

	ldr ip, =_interrupt_stack
	add.w ip, #(__tESF_SIZEOF * 2) /* enough for a stack frame */
	ldr r1, =0xfffffffe
	and.w r2, r1
	str r2, [ip, #(6 * 4)]
	ldr r2, =0x01000000
	str r2, [ip, #(7 * 4)]

	ite eq
		moveq sp, ip
		msrne PSP, ip

	bx lr

#endif
