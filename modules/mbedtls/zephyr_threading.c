/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <mbedtls/threading.h>
#include <zephyr_mbedtls_priv.h>

static int zephyr_mbedtls_mutex_init(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_init(mutex) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static void zephyr_mbedtls_mutex_destroy(mbedtls_platform_mutex_t *mutex)
{
	ARG_UNUSED(mutex);
}

static int zephyr_mbedtls_mutex_lock(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_lock(mutex, K_FOREVER) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int zephyr_mbedtls_mutex_unlock(mbedtls_platform_mutex_t *mutex)
{
	if (k_mutex_unlock(mutex) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int zephyr_mbedtls_cond_init(mbedtls_platform_condition_variable_t *condvar)
{
	if (k_condvar_init(condvar) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static void zephyr_mbedtls_cond_destroy(mbedtls_platform_condition_variable_t *condvar)
{
	ARG_UNUSED(condvar);
}

static int zephyr_mbedtls_cond_signal(mbedtls_platform_condition_variable_t *condvar)
{
	if (k_condvar_signal(condvar) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int zephyr_mbedtls_cond_broadcast(mbedtls_platform_condition_variable_t *condvar)
{
	/*
	 * k_condvar_broadcast() returns the non-negative count of woken
	 * threads on success, whereas Mbed TLS expects 0 on success.
	 */
	if (k_condvar_broadcast(condvar) < 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

static int zephyr_mbedtls_cond_wait(mbedtls_platform_condition_variable_t *condvar,
				    mbedtls_platform_mutex_t *mutex)
{
	if (k_condvar_wait(condvar, mutex, K_FOREVER) != 0) {
		return MBEDTLS_ERR_THREADING_USAGE_ERROR;
	}

	return 0;
}

void zephyr_mbedtls_init_threading(void)
{
	mbedtls_threading_set_alt(zephyr_mbedtls_mutex_init, zephyr_mbedtls_mutex_destroy,
				  zephyr_mbedtls_mutex_lock, zephyr_mbedtls_mutex_unlock,
				  zephyr_mbedtls_cond_init, zephyr_mbedtls_cond_destroy,
				  zephyr_mbedtls_cond_signal, zephyr_mbedtls_cond_broadcast,
				  zephyr_mbedtls_cond_wait);
}
