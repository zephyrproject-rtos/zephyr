/*
 * Copyright (c) 2019 Linaro. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr.h>
#include <misc/printk.h>

#include "tfm_api.h"
#include "tfm_ns_lock.h"

K_MUTEX_DEFINE(tfm_mutex);

/**
 * @brief Non-secure processing environment lock dispatcher.
 *
 * @param fn The veneer function to call.
 * @param arg0 Veneer function arg 0.
 * @param arg1 Veneer function arg 1.
 * @param arg2 Veneer function arg 2.
 * @param arg3 Veneer function arg 3.
 *
 * @return 0 on success, TFM_ERROR_GENERIC if the mutex lock failed,
 * otherwise and appropriate TFM error code.
 */
uint32_t tfm_ns_lock_dispatch(veneer_fn fn,
			      u32_t arg0, u32_t arg1,
			      u32_t arg2, u32_t arg3)
{
	u32_t result;

	/* TFM request protected by NS lock */
	if (k_mutex_lock(&tfm_mutex, K_MSEC(1000)) == 0) {
		/* Mutex successfully locked. */
		result = fn(arg0, arg1, arg2, arg3);
		k_mutex_unlock(&tfm_mutex);
	} else {
		/* Cannot lock TFM mutex. */
		printk("Unable to set TFM mutex!\n");
		return TFM_ERROR_GENERIC;
	}

	return result;
}

/**
 * @brief Non-secure processing environment lock init.
 *
 * @return TFM_SUCCESS is always returned by this function.
 */
enum tfm_status_e tfm_ns_lock_init(void)
{
	/* Mutex initialised at compile time. */
	return TFM_SUCCESS;
}
