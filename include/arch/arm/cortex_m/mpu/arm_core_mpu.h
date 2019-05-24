/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_

#include <kernel_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Use the HW-specific MPU driver to program
 *        the static MPU regions.
 *
 * Program the static MPU regions through the HW-specific
 * MPU driver. The function is meant to be invoked once,
 * during system initialization.
 */
void z_arch_configure_static_mpu_regions(void);

/**
 * @brief Use the HW-specific MPU driver to program
 *        the dynamic MPU regions.
 *
 * Program the dynamic MPU regions using the HW-specific MPU
 * driver. This function is meant to be invoked every time the
 * memory map is to be re-programmed, e.g during thread context
 * switch, entering user mode, reconfiguring memory domain, etc.
 *
 * @param thread pointer to the current k_thread context
 */
void z_arch_configure_dynamic_mpu_regions(struct k_thread *thread);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_CORTEX_M_MPU_ARM_CORE_MPU_H_ */
