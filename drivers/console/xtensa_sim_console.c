/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/simcall.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)
/**
 * @brief Output one character to SIMULATOR console
 * @param c Character to output
 * @return The character passed as input.
 */
int arch_printk_char_out(int c)
{
	char buf[16];

	register int a2 __asm__ ("a2") = SYS_write;
	register int a3 __asm__ ("a3") = 1;
	register char *a4 __asm__ ("a4") = buf;
	register int a5 __asm__ ("a5") = 1;
	register int ret_val __asm__ ("a2");
	register int ret_err __asm__ ("a3");

	buf[0] = (char)c;
	__asm__ volatile ("simcall"
				: "=a" (ret_val), "=a" (ret_err)
				: "a" (a2), "a" (a3), "a" (a4), "a" (a5)
				: "memory");
	return c;
}
#endif

/**
 * @brief Install printk/stdout hook for Xtensa Simulator console output
 */
static void xt_sim_console_hook_install(void)
{
#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(arch_printk_char_out);
#endif
#if defined(CONFIG_PRINTK)
	__printk_hook_install(arch_printk_char_out);
#endif
}

/**
 * @brief Initialize the console/debug port
 * @return 0 if successful, otherwise failed.
 */
static int xt_sim_console_init(void)
{
	xt_sim_console_hook_install();
	return 0;
}

/* UART console initializes after the UART device itself */
SYS_INIT(xt_sim_console_init,
#if defined(CONFIG_EARLY_CONSOLE)
	 PRE_KERNEL_1,
#else
	 POST_KERNEL,
#endif
	 CONFIG_CONSOLE_INIT_PRIORITY);
