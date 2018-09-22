/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_UART_H__
#define SHELL_UART_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_uart_transport_api;

struct shell_uart {
	struct device *dev;
	shell_transport_handler_t handler;
	struct k_timer timer;
	void *context;
	u8_t rx[1];
	size_t rx_cnt;
};

#define SHELL_UART_DEFINE(_name)					\
	static struct shell_uart _name##_shell_uart;			\
	struct shell_transport _name = {				\
		.api = &shell_uart_transport_api,			\
		.ctx = (struct shell_uart *)&_name##_shell_uart		\
	}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UART_H__ */
