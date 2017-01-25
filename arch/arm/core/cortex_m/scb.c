/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM CORTEX-M3 System Control Block interface
 *
 *
 * Most of the SCB interface consists of simple bit-flipping methods, and is
 * implemented as inline functions in scb.h. This module thus contains only data
 * definitions and more complex routines, if needed.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/util.h>
#include <arch/arm/cortex_m/cmsis.h>

#if defined(CONFIG_SOC_TI_LM3S6965_QEMU)
/*
 * QEMU is missing the support for rebooting through the SYSRESETREQ mechanism.
 * Just jump back to __reset() of the image in flash, which address can
 * _always_ be found in the vector table reset slot located at address 0x4.
 */

static void software_reboot(void)
{
	extern void _do_software_reboot(void);
	extern void _force_exit_one_nested_irq(void);
	/*
	 * force enable interrupts locked via PRIMASK if somehow disabled: the
	 * boot code does not enable them
	 */
	__asm__ volatile("cpsie i" :::);

	if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == 0) {
		_do_software_reboot();
	} else {
		__asm__ volatile(
			"ldr r0,  =_force_exit_one_nested_irq\n\t"
			"bx r0\n\t"
			:::);
	}
}
#define DO_REBOOT() software_reboot()
#else
#define DO_REBOOT() NVIC_SystemReset()
#endif

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 * @return N/A
 */

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
	DO_REBOOT();
}
