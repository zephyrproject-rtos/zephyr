/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

static const struct device *const uart_console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

void uart_tccvcp_poll_out(const struct device *dev, unsigned char c);

static int arch_printk_char_out(int c)
{
	if (!device_is_ready(uart_console_dev)) {
		return -ENODEV;
	}

	if ('\n' == c) {
		uart_tccvcp_poll_out(uart_console_dev, '\r');
	}
	uart_tccvcp_poll_out(uart_console_dev, c);

	return c;
}

static void tccvcp_console_hook_install(void)
{
#if defined(CONFIG_PRINTK)
	__printk_hook_install(arch_printk_char_out);
#endif
	__stdout_hook_install(arch_printk_char_out);
}

static int tccvcp_console_init(void)
{
	if (!device_is_ready(uart_console_dev)) {
		return -ENODEV;
	}

	tccvcp_console_hook_install();

	return 0;
}

SYS_INIT(tccvcp_console_init, POST_KERNEL, CONFIG_CONSOLE_INIT_PRIORITY);
