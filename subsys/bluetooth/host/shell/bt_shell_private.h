/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>

#ifndef __BT_SHELL_PRIVATE_H
#define __BT_SHELL_PRIVATE_H

/**
 * @brief printf-like function which sends formatted data stream to the shell.
 * (Bluetooth context specific)
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context.
 *
 * @param[in] fmt   Format string.
 * @param[in] ...   List of parameters to print.
 */
void __printf_like(1, 2) bt_shell_info_impl(const char *fmt, ...);
void __printf_like(1, 2) bt_shell_print_impl(const char *fmt, ...);
void __printf_like(1, 2) bt_shell_warn_impl(const char *fmt, ...);
void __printf_like(1, 2) bt_shell_error_impl(const char *fmt, ...);

/**
 * @brief Print info message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_info(_ft, ...) \
	bt_shell_info_impl(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print normal message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_print(_ft, ...) \
	bt_shell_print_impl(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print warning message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_warn(_ft, ...) \
	bt_shell_warn_impl(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print error message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_error(_ft, ...) \
	bt_shell_error_impl(_ft "\n", ##__VA_ARGS__)

#define bt_shell_fprintf_info(_ft, ...) \
	bt_shell_info_impl(_ft, ##__VA_ARGS__)

#define bt_shell_fprintf_print(_ft, ...) \
	bt_shell_print_impl(_ft, ##__VA_ARGS__)

#define bt_shell_fprintf_warn(_ft, ...) \
	bt_shell_warn_impl(_ft, ##__VA_ARGS__)

#endif /* __BT_SHELL_PRIVATE_H */
