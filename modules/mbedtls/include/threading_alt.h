/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#include <zephyr/kernel.h>
#include <zephyr/kernel/thread.h>

typedef struct k_mutex mbedtls_platform_mutex_t;
typedef struct k_condvar mbedtls_platform_condition_variable_t;

#endif /* MBEDTLS_THREADING_ALT_H */
