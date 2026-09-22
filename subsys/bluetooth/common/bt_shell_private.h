/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_SHELL_PRIVATE_H
#define __BT_SHELL_PRIVATE_H

#include <zephyr/shell/shell.h>

/**
 * @brief printf-like function which sends formatted data stream to the shell.
 * (Bluetooth context specific)
 *
 * This function can be used from the command handler or from threads, but not
 * from an interrupt context.
 *
 * @param[in] color Printed text color.
 * @param[in] fmt   Format string.
 * @param[in] ...   List of parameters to print.
 */
__printf_like(2, 3) void bt_shell_fprintf(enum shell_vt100_color color,
					  const char *fmt, ...);

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
__printf_like(1, 2) void bt_shell_fprintf_info(const char *fmt, ...);
__printf_like(1, 2) void bt_shell_fprintf_print(const char *fmt, ...);
__printf_like(1, 2) void bt_shell_fprintf_warn(const char *fmt, ...);
__printf_like(1, 2) void bt_shell_fprintf_error(const char *fmt, ...);

/**
 * @brief Print data in hexadecimal format.
 * (Bluetooth context specific)
 *
 * @param[in] data Pointer to data.
 * @param[in] len  Length of data.
 */
void bt_shell_hexdump(const uint8_t *data, size_t len);

/**
 * @brief Print info message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_info(_ft, ...) \
	bt_shell_fprintf_info(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print normal message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_print(_ft, ...) \
	bt_shell_fprintf_print(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print warning message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_warn(_ft, ...) \
	bt_shell_fprintf_warn(_ft "\n", ##__VA_ARGS__)

/**
 * @brief Print error message to the shell.
 * (Bluetooth context specific)
 *
 * @param[in] _ft Format string.
 * @param[in] ... List of parameters to print.
 */
#define bt_shell_error(_ft, ...) \
	bt_shell_fprintf_error(_ft "\n", ##__VA_ARGS__)

#endif /* __BT_SHELL_PRIVATE_H */
