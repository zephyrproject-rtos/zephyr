/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <cmsis_os.h>

/**
 * @brief Wait for Timeout (Time Delay in ms).
 */
osStatus osDelay(uint32_t delay_ms)
{
	if (k_is_in_isr()) {
		return osErrorISR;
	}

	k_msleep(delay_ms);
	return osEventTimeout;
}
