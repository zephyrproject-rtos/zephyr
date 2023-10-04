/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NMI handler infrastructure
 *
 * Provides a boot time handler that simply hangs in a sleep loop, and a run
 * time handler that resets the CPU. Also provides a mechanism for hooking a
 * custom run time handler.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

extern void z_SysNmiOnReset(void);
#if !defined(CONFIG_RUNTIME_NMI)
#define handler z_SysNmiOnReset
#endif

#ifdef CONFIG_RUNTIME_NMI
typedef void (*_NmiHandler_t)(void);
static _NmiHandler_t handler = z_SysNmiOnReset;

/**
 *
 * @brief Default NMI handler installed when kernel is up
 *
 * The default handler outputs a error message and reboots the target. It is
 * installed by calling z_arm_nmi_init();
 *
 */

static void DefaultHandler(void)
{
	printk("NMI received! Rebooting...\n");
	/* In ARM implementation sys_reboot ignores the parameter */
	sys_reboot(0);
}

/**
 *
 * @brief Install default runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a simple NMI
 * handler that reboots the target. It should be installed after the console is
 * initialized.
 *
 */

void z_arm_nmi_init(void)
{
	handler = DefaultHandler;
}

/**
 *
 * @brief Install a custom runtime NMI handler
 *
 * Meant to be called by platform code if they want to install a custom NMI
 * handler that reboots. It should be installed after the console is
 * initialized if it is meant to output to the console.
 *
 */

void z_arm_nmi_set_handler(void (*pHandler)(void))
{
	handler = pHandler;
}
#endif /* CONFIG_RUNTIME_NMI */

/**
 *
 * @brief Handler installed in the vector table
 *
 * Simply call what is installed in 'static void(*handler)(void)'.
 *
 */

void z_arm_nmi(void)
{
	handler();
	z_arm_int_exit();
}
