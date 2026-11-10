/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MBEDTLS_THREADING_ALT_H
#define ZEPHYR_MBEDTLS_THREADING_ALT_H

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct k_mutex mbedtls_platform_mutex_t;
typedef struct k_condvar mbedtls_platform_condition_variable_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MBEDTLS_THREADING_ALT_H */
