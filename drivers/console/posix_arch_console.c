/*
 * Copyright (c) 2018 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/arch/posix/posix_trace.h>
#include <zephyr/sys/printk-hooks.h>

#define _STDOUT_BUF_SIZE 256
static char stdout_buff[_STDOUT_BUF_SIZE];
static int n_pend; /* Number of pending characters in buffer */

static int print_char(int c)
{
	int printnow = 0;

	if ((c != '\n') && (c != '\r')) {
		stdout_buff[n_pend++] = c;
		stdout_buff[n_pend] = 0;
	} else {
		printnow = 1;
	}

	if (n_pend >= _STDOUT_BUF_SIZE - 1) {
		printnow = 1;
	}

	if (printnow) {
		posix_print_trace("%s\n", stdout_buff);
		n_pend = 0;
		stdout_buff[0] = 0;
	}
	return c;
}

/**
 * Ensure that whatever was written thru printk is displayed now
 */
void posix_flush_stdout(void)
{
	if (n_pend) {
		stdout_buff[n_pend] = 0;
		posix_print_trace("%s", stdout_buff);
		n_pend = 0;
		stdout_buff[0] = 0;
	}
}

static int posix_arch_console_init(void)
{
#ifdef CONFIG_PRINTK
	__printk_hook_install(print_char);
#endif
#ifdef CONFIG_STDOUT_CONSOLE
	extern void __stdout_hook_install(int (*fn)(int));
	__stdout_hook_install(print_char);
#endif
	return 0;
}

SYS_INIT(posix_arch_console_init, PRE_KERNEL_1,
	CONFIG_POSIX_ARCH_CONSOLE_INIT_PRIORITY);
