/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>

#ifdef CONFIG_UART_CONSOLE
/* Support early console log */
int arch_printk_char_out(int _c)
{
	uart_poll_out(DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart)), (char)_c);
	return 0;
}
#endif /* CONFIG_UART_CONSOLE */
