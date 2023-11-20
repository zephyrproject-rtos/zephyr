/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_UART_H__
#define SHELL_UART_H__

#include <zephyr/mgmt/mcumgr/transport/smp_shell.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function provides pointer to the shell UART backend instance.
 *
 * Function returns pointer to the shell UART instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_uart_get_ptr(void);

/**
 * @brief This function provides pointer to the smp shell data of the UART shell transport.
 *
 * @returns Pointer to the smp shell data.
 */
struct smp_shell_data *shell_uart_smp_shell_data_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_UART_H__ */
