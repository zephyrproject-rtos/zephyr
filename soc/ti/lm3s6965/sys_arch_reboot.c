/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <cmsis_core.h>

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 */

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

    /*
     * QEMU is missing the support for rebooting through the SYSRESETREQ
     * mechanism.  Just jump back to __reset() of the image in flash,
     * which address can _always_ be found in the vector table reset slot
     * located at address 0x4.
     */
	extern void z_do_software_reboot(void);
	extern void z_force_exit_one_nested_irq(void);
	/*
	 * force enable interrupts locked via PRIMASK if somehow disabled: the
	 * boot code does not enable them
	 */
	__asm__ volatile("cpsie i" :::);

	if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == 0) {
		z_do_software_reboot();
	} else {
		__asm__ volatile(
			"ldr r0,  =z_force_exit_one_nested_irq\n\t"
			"bx r0\n\t"
			:::);
	}
}
