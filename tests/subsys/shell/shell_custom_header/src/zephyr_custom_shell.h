/*
 * Copyright (c) 2024 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CUSTOM_SHELL_H
#define __ZEPHYR_CUSTOM_SHELL_H

#define CUSTOM_SHELL_PREFIX "[CUSTOM_PREFIX]"

#undef shell_fprintf

#define shell_fprintf(sh, color, fmt, ...)                                                         \
	shell_fprintf_impl(sh, color, CUSTOM_SHELL_PREFIX fmt, ##__VA_ARGS__)

#endif /* __ZEPHYR_CUSTOM_SHELL_H */
