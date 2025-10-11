/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <cmsis_os.h>
#include <kernel_internal.h>

/**
 * @brief Get the RTOS kernel system timer counter
 */
uint32_t osKernelSysTick(void)
{
	return k_cycle_get_32();
}

/**
 * @brief Initialize the RTOS Kernel for creating objects.
 */
osStatus osKernelInitialize(void)
{
	return osOK;
}

/**
 * @brief Start the RTOS Kernel.
 */
osStatus osKernelStart(void)
{
	if (k_is_in_isr()) {
		return osErrorISR;
	}
	return osOK;
}

/**
 * @brief Check if the RTOS kernel is already started.
 */
int32_t osKernelRunning(void)
{
	return !z_is_thread_suspended(&z_main_thread);
}
