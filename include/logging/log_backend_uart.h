/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_BACKEND_UART_H
#define LOG_BACKEND_UART_H

#include <logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct log_backend_api log_backend_uart_api;

/**
 * @brief UART backend definition
 *
 * @param _name Name of the instance.
 */
#define LOG_BACKEND_UART_DEFINE(_name) \
	LOG_BACKEND_DEFINE(_name, log_backend_uart_api)

#ifdef __cplusplus
}
#endif

void log_backend_uart_init(void);

#endif /* LOG_BACKEND_UART_H */
