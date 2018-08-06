/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/cli.h>
#include <shell/shell_uart.h>
#include <shell/shell_rtt.h>
#include <version.h>

#if CONFIG_SHELL
void test_cmd(const struct shell *shell, size_t argc, char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL, "hello\r\n");
}
SHELL_UART_DEFINE(shell_transport_uart);
SHELL_RTT_DEFINE(shell_transport_rtt);
SHELL_CMD_REGISTER(test, NULL, "help", test_cmd);

SHELL_DEF(uart_shell, "uart:~$ ", &shell_transport_uart, '\r', 10);
SHELL_DEF(rtt_shell, "rtt:~$ ", &shell_transport_rtt, '\r', 10);
#endif

void main(void)
{
	//(void)shell_init(&uart_shell, NULL, true, false, 0);
#if CONFIG_SHELL
	(void)shell_init(&rtt_shell, NULL, true, false, 0);
#endif
}
