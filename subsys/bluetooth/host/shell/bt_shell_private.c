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

#include "bt_shell_private.h"

extern const struct shell *ctx_shell;

void bt_shell_info_impl(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_vfprintf(ctx_shell, SHELL_INFO, fmt, args);
	va_end(args);
}

void bt_shell_print_impl(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_vfprintf(ctx_shell, SHELL_NORMAL, fmt, args);
	va_end(args);
}


void bt_shell_warn_impl(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_vfprintf(ctx_shell, SHELL_WARNING, fmt, args);
	va_end(args);
}

void bt_shell_error_impl(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	shell_vfprintf(ctx_shell, SHELL_ERROR, fmt, args);
	va_end(args);
}
