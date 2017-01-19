/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for NSIM UART driver
 */

#ifndef _DRIVERS_UART_NSIM_H_
#define _DRIVERS_UART_NSIM_H_

#include <uart.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_nsim_port_init(struct device *, const struct uart_init_info * const);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVERS_UART_NSIM_H_ */
