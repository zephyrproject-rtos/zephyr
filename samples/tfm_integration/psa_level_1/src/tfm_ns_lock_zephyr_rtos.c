/*
 * Copyright (c) 2017-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
 * \brief NS world, NS lock based dispatcher
 */
uint32_t tfm_ns_lock_dispatch(veneer_fn fn,
			      uint32_t arg0, uint32_t arg1,
			      uint32_t arg2, uint32_t arg3)
{
	uint32_t result;

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
 * \brief NS world, Init NS lock
 */
enum tfm_status_e tfm_ns_lock_init()
{
	/* Mutex initialised at compile time. */
	return TFM_SUCCESS;
}
