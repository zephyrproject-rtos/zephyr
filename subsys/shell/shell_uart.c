/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell_uart.h>
#include <uart.h>
#include <init.h>

SHELL_UART_DEFINE(shell_transport_uart);
SHELL_DEFINE(uart_shell, "uart:~$ ", &shell_transport_uart, 10,
	     SHELL_FLAG_OLF_CRLF);

static void timer_handler(struct k_timer *timer)
{
	struct shell_uart *sh_uart =
			CONTAINER_OF(timer, struct shell_uart, timer);

	if (uart_poll_in(sh_uart->dev, sh_uart->rx) == 0) {
		sh_uart->rx_cnt = 1;
		sh_uart->handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_uart->context);
	}
}


static int init(const struct shell_transport *transport,
		const void *config,
		shell_transport_handler_t evt_handler,
		void *context)
{
	struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	sh_uart->dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
	sh_uart->handler = evt_handler;
	sh_uart->context = context;

	k_timer_init(&sh_uart->timer, timer_handler, NULL);

	k_timer_start(&sh_uart->timer, 20, 20);

	return 0;
}

static int uninit(const struct shell_transport *transport)
{
	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking)
{
	return 0;
}

static int write(const struct shell_transport *transport,
		 const void *data, size_t length, size_t *cnt)
{
	struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;
	const u8_t *data8 = (const u8_t *)data;

	for (size_t i = 0; i < length; i++) {
		uart_poll_out(sh_uart->dev, data8[i]);
	}

	*cnt = length;

	sh_uart->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_uart->context);

	return 0;
}

static int read(const struct shell_transport *transport,
		void *data, size_t length, size_t *cnt)
{
	struct shell_uart *sh_uart = (struct shell_uart *)transport->ctx;

	if (sh_uart->rx_cnt) {
		memcpy(data, sh_uart->rx, 1);
		sh_uart->rx_cnt = 0;
		*cnt = 1;
	} else {
		*cnt = 0;
	}

	return 0;
}

const struct shell_transport_api shell_uart_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write,
	.read = read
};

static int enable_shell_uart(struct device *arg)
{
	ARG_UNUSED(arg);
	shell_init(&uart_shell, NULL, true, true, LOG_LEVEL_INF);
	return 0;
}
SYS_INIT(enable_shell_uart, POST_KERNEL, 0);
