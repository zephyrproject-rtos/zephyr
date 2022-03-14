/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_STRING_CONV_H__
#define ZEPHYR_INCLUDE_SYS_STRING_CONV_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert string to long param.
 *
 * @note On failure the passed value reference will not be altered.
 *
 * @param str Input string
 * @param val Converted value
 *
 * @return 0 on success.
 * @return -EINVAL on invalid string input.
 * @return -ERANGE if numeric string input is to large to convert.
 */
int string_conv_str2long(const char *str, long *val);

/**
 * @brief Convert string to unsigned long param.
 *
 * @note On failure the passed value reference will not be altered.
 *
 * @param str Input string
 * @param val Converted value
 *
 * @return 0 on success.
 * @return -EINVAL on invalid string input.
 * @return -ERANGE if numeric string input is to large to convert.
 */
int string_conv_str2ulong(const char *str, unsigned long *val);

/**
 * @brief Convert string to double param.
 *
 * @note On failure the passed value reference will not be altered.
 *
 * @param str Input string
 * @param val Converted value
 *
 * @return 0 on success.
 * @return -EINVAL on invalid string input.
 * @return -ERANGE if numeric string input is to large to convert.
 */
int string_conv_str2dbl(const char *str, double *val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_STRING_CONV_H__ */
