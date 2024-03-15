/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "modem_backend_uart_isr.h"
#include "modem_backend_uart_async.h"

#include <zephyr/modem/backend/uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_backend_uart, CONFIG_MODEM_MODULES_LOG_LEVEL);

#include <string.h>

static void modem_backend_uart_receive_ready_handler(struct k_work *item)
{
	struct modem_backend_uart *backend =
		CONTAINER_OF(item, struct modem_backend_uart, receive_ready_work);

	modem_pipe_notify_receive_ready(&backend->pipe);
}

static void modem_backend_uart_transmit_idle_handler(struct k_work *item)
{
	struct modem_backend_uart *backend =
		CONTAINER_OF(item, struct modem_backend_uart, transmit_idle_work);

	modem_pipe_notify_transmit_idle(&backend->pipe);
}

struct modem_pipe *modem_backend_uart_init(struct modem_backend_uart *backend,
					   const struct modem_backend_uart_config *config)
{
	__ASSERT_NO_MSG(config->uart != NULL);
	__ASSERT_NO_MSG(config->receive_buf != NULL);
	__ASSERT_NO_MSG(config->receive_buf_size > 1);
	__ASSERT_NO_MSG((config->receive_buf_size % 2) == 0);
	__ASSERT_NO_MSG(config->transmit_buf != NULL);
	__ASSERT_NO_MSG(config->transmit_buf_size > 0);

	memset(backend, 0x00, sizeof(*backend));
	backend->uart = config->uart;
	k_work_init(&backend->receive_ready_work, modem_backend_uart_receive_ready_handler);
	k_work_init(&backend->transmit_idle_work, modem_backend_uart_transmit_idle_handler);

#ifdef CONFIG_MODEM_BACKEND_UART_ASYNC
	if (modem_backend_uart_async_is_supported(backend)) {
		modem_backend_uart_async_init(backend, config);
		return &backend->pipe;
	}
#endif /* CONFIG_MODEM_BACKEND_UART_ASYNC */

#ifdef CONFIG_MODEM_BACKEND_UART_ISR
	modem_backend_uart_isr_init(backend, config);

	return &backend->pipe;
#endif /* CONFIG_MODEM_BACKEND_UART_ISR */

	__ASSERT(0, "No supported UART API");

	return NULL;
}
