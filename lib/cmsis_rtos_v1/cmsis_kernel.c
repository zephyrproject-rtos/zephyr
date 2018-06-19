/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <cmsis_os.h>

/**
 * @brief Get the RTOS kernel system timer counter
 */
uint32_t osKernelSysTick(void)
{
	return k_cycle_get_32();
}
