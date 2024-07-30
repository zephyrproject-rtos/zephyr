/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/backend/uart.h>

#ifndef ZEPHYR_MODEM_BACKEND_UART_ASYNC_
#define ZEPHYR_MODEM_BACKEND_UART_ASYNC_

#ifdef __cplusplus
extern "C" {
#endif

bool modem_backend_uart_async_is_supported(struct modem_backend_uart *backend);

void modem_backend_uart_async_init(struct modem_backend_uart *backend,
				   const struct modem_backend_uart_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_BACKEND_UART_ASYNC_ */
