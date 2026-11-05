/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/printk-hooks.h>

static printk_hook_fn_t console_out;
static bool line_start = true;

static int prefixed_out(int character)
{
	static const char *const prefix =
		IS_ENABLED(CONFIG_MC_UART_SENDER) || IS_ENABLED(CONFIG_TEST_MC_STRESS_SENDER)
			? "[SENDER] "
			: "[RECEIVER] ";

	if (line_start) {
		for (const char *p = prefix; *p != '\0'; p++) {
			(void)console_out(*p);
		}
		line_start = false;
	}
	(void)console_out(character);
	if (character == '\n') {
		line_start = true;
	}
	return character;
}

static int install_console_prefix(void)
{
	console_out = __printk_get_hook();
	if (console_out == NULL) {
		return -ENODEV;
	}
	__printk_hook_install(prefixed_out);
	__stdout_hook_install(prefixed_out);
	return 0;
}

SYS_INIT(install_console_prefix, POST_KERNEL, 99);
