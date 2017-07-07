/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ARM_CORE_MPU_H_
#define _ARM_CORE_MPU_H_

#include <kernel_structs.h>

#if defined(CONFIG_ARM_MPU)
#include <arch/arm/cortex_m/mpu/arm_mpu.h>
#elif defined(CONFIG_NXP_MPU)
#include <arch/arm/cortex_m/mpu/nxp_mpu.h>
#else
#error "Unsupported MPU"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MPU_STACK_GUARD)
/**
 * @brief Configure MPU stack guard
 *
 * This function configures per thread stack guards reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_stack_guard(struct k_thread *thread);
#endif

#if defined(CONFIG_USERSPACE)
/*
 * @brief Configure MPU memory domain
 *
 * This function configures per thread memory domain reprogramming the MPU.
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_mpu_mem_domain(struct k_thread *thread);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORE_MPU_H */
