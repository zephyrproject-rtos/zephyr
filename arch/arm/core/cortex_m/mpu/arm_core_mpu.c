/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include <arch/arm/cortex_m/cmsis.h>
#include <arch/arm/cortex_m/mpu/arm_core_mpu.h>

#if defined(CONFIG_MPU_STACK_GUARD)
/*
 * @brief Configure MPU stack guard
 *
 * This function configures per thread stack guards reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_stack_guard(struct k_thread *thread)
{
	arm_core_mpu_disable();
	arm_core_mpu_configure(THREAD_STACK_GUARD_REGION,
			       thread->stack_info.start,
			       thread->stack_info.size);
	arm_core_mpu_enable();
}
#endif
