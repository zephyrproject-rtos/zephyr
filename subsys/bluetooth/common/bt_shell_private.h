/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_SHELL_PRIVATE_H
#define __BT_SHELL_PRIVATE_H

#include <stddef.h>

#include <zephyr/bluetooth/addr.h>
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

/**
 * @brief Parse a Bluetooth LE address from shell argv.
 *
 * Accepts both the prefix-based string format ("P:XX:XX:XX:XX:XX:XX" or
 * "R:XX:XX:XX:XX:XX:XX") which consumes a single argv slot, and the legacy
 * two-token format ("XX:XX:XX:XX:XX:XX" followed by a "public" / "random" /
 * "public-id" / "random-id" type token) which consumes two slots.
 *
 * The new prefix format is attempted first. If it does not match, and at
 * least one further argv slot is available beyond @p start, the legacy
 * format is attempted with @p argv[start + 1] as the type token.
 *
 * @param[in]  argc  Total argument count provided to the shell handler.
 * @param[in]  argv  Argument vector provided to the shell handler.
 * @param[in]  start Index of the first argv slot to parse.
 * @param[out] addr  Parsed address on success.
 *
 * @retval 1 The prefix-based address was parsed successfully.
 * @retval 2 The legacy address and type were parsed successfully.
 * @retval -EINVAL The address argument is missing or invalid.
 */
int bt_shell_strtoaddr_le(size_t argc, char *argv[], size_t start, bt_addr_le_t *addr);

#endif /* __BT_SHELL_PRIVATE_H */
