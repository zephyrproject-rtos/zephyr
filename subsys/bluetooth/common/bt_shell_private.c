/**
 * @file bt_shell_private.c
 * @brief Bluetooth shell private module
 *
 * Provide common function which can be shared using privately inside Bluetooth shell.
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell_backend.h>
#include "bt_shell_private.h"

static void wall_vfprintf(enum shell_vt100_color color, const char *fmt, va_list args)
{
	int count;
	const struct shell *sh;

	count = shell_backend_count_get();
	for (int i = 0; i < count; i++) {
		va_list args_copy;

		va_copy(args_copy, args);   /* Create a copy of 'args' to safely reuse */
		sh = shell_backend_get(i);
		shell_vfprintf(sh, color, fmt, args_copy);
		va_end(args_copy);          /* Clean up to prevent resource leaks */
	}
}

void bt_shell_hexdump(const uint8_t *data, size_t len)
{
	int count;
	const struct shell *sh;

	count = shell_backend_count_get();
	for (int i = 0; i < count; i++) {
		sh = shell_backend_get(i);
		shell_hexdump(sh, data, len);
	}
}

void bt_shell_fprintf(enum shell_vt100_color color,
		      const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wall_vfprintf(color, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_info(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wall_vfprintf(SHELL_INFO, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_print(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wall_vfprintf(SHELL_NORMAL, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_warn(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wall_vfprintf(SHELL_WARNING, fmt, args);
	va_end(args);
}

void bt_shell_fprintf_error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wall_vfprintf(SHELL_ERROR, fmt, args);
	va_end(args);
}
