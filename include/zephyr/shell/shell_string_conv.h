/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_STRING_CONV_H__
#define SHELL_STRING_CONV_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief String to long conversion with error check.
 *
 * @warning On success the passed err reference will not be altered
 * to avoid err check bloating. Passed err reference should be initialized
 * to zero.
 *
 * @param str Input string.
 * @param base Conversion base.
 * @param err Error code pointer:
 *         -EINVAL on invalid string input.
 *         -ERANGE if numeric string input is to large to convert.
 *         Unchanged on success.
 *
 * @return Converted long value.
 */
long shell_strtol(const char *str, int base, int *err);

/** @brief String to unsigned long conversion with error check.
 *
 * @warning On success the passed err reference will not be altered
 * to avoid err check bloating. Passed err reference should be initialized
 * to zero.
 *
 * @param str Input string.
 * @param base Conversion base.
 * @param err Error code pointer:
 *         Set to -EINVAL on invalid string input.
 *         Set to -ERANGE if numeric string input is to large to convert.
 *         Unchanged on success.
 *
 * @return Converted unsigned long value.
 */
unsigned long shell_strtoul(const char *str, int base, int *err);

/** @brief String to boolean conversion with error check.
 *
 * @warning On success the passed err reference will not be altered
 * to avoid err check bloating. Passed err reference should be initialized
 * to zero.
 *
 * @param str Input string.
 * @param base Conversion base.
 * @param err Error code pointer:
 *         Set to -EINVAL on invalid string input.
 *         Set to -ERANGE if numeric string input is to large to convert.
 *         Unchanged on success.
 *
 * @return Converted boolean value.
 */
bool shell_strtobool(const char *str, int base, int *err);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_STRING_CONV_H__ */
