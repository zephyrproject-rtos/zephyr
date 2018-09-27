/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_

#include <kernel_structs.h>

#if defined(CONFIG_CPU_HAS_ARM_MPU)
#include <arch/arm/cortex_m/mpu/arm_mpu.h>
#elif defined(CONFIG_CPU_HAS_NXP_MPU)
#include <arch/arm/cortex_m/mpu/nxp_mpu.h>
#else
#error "Unsupported MPU"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MPU_REQUIRES_NON_OVERLAPPING_REGIONS)
/**
 * @brief Reset the access permissions of the entire kernel SRAM region
 *
 * This function will reset the access permissions of the kernel RAM region.
 * It does this by re-programming the MPU region that covers the entire kernel
 * SRAM (i.e. KERNEL_BKGND_REGION). In addition, it clears any MPU regions
 * related to memory protection features (e.g. THREAD_STACK, STACK_GUARD, etc.)
 * and their respective background ram regions. The function intends to be used
 * during context switch, for MPU architectures that do not allow overlapping
 * active MPU regions.
 */
void configure_mpu_kernel_ram_region_reset(void);
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

/*
 * @brief Configure MPU user context
 *
 * This function configures the stack and application data regions
 * for user mode threads
 *
 * @param thread thread info data structure.
 */
void configure_mpu_user_context(struct k_thread *thread);

#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_ */
