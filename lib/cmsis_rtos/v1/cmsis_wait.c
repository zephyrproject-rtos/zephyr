/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <cmsis_os.h>

/**
 * @brief Wait for Timeout (Time Delay in ms).
 */
osStatus osDelay(uint32_t delay_ms)
{
	if (k_is_in_isr()) {
		return osErrorISR;
	}

	k_sleep(delay_ms);
	return osEventTimeout;
}
