/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/**
 * @brief Output one character to SIMULATOR console
 * @param c Character to output
 * @return The character passed as input.
 */
static int console_out(int c)
{
	register unsigned long x0 __asm__("x0") = 8;
	register unsigned long x1 __asm__("x1") = c;

	__asm__ volatile ("hvc #0x4a48\r\n"
			  : "+r" (x0), "+r" (x1) : : );
	return c;
}
#endif

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)		\
	do {/* nothing */			\
	} while ((0))
#endif

/**
 * @brief Initialize the console/debug port
 * @return 0 if successful, otherwise failed.
 */
static int jailhouse_console_init(void)
{
	__stdout_hook_install(console_out);
	__printk_hook_install(console_out);
	return 0;
}

SYS_INIT(jailhouse_console_init,
	 PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
