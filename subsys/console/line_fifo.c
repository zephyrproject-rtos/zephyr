/*
 * Copyright (c) 2018 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Legacy fifo-based line input
 */

#include <zephyr.h>
#include <console.h>

#ifdef CONFIG_UART_CONSOLE
#include <drivers/console/uart_console.h>
#endif
#ifdef CONFIG_TELNET_CONSOLE
#include <drivers/console/telnet_console.h>
#endif
#ifdef CONFIG_NATIVE_POSIX_CONSOLE
#include <drivers/console/native_posix_console.h>
#endif
#ifdef CONFIG_WEBSOCKET_CONSOLE
#include <drivers/console/websocket_console.h>
#endif

void console_register_line_input(struct k_fifo *avail_queue,
				 struct k_fifo *out_queue,
				 u8_t (*completion)(char *str, u8_t len))
{
	/* Register serial console handler */
#ifdef CONFIG_UART_CONSOLE
	uart_register_input(avail_queue, out_queue, completion);
#endif
#ifdef CONFIG_TELNET_CONSOLE
	telnet_register_input(avail_queue, out_queue, completion);
#endif
#ifdef CONFIG_NATIVE_POSIX_STDIN_CONSOLE
	native_stdin_register_input(avail_queue, out_queue, completion);
#endif
#ifdef CONFIG_WEBSOCKET_CONSOLE
	ws_register_input(avail_queue, out_queue, completion);
#endif
}
