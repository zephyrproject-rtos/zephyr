/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Early boot init
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EARLY_INIT_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EARLY_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function called at boot before C setup code has ran.
 *
 * This should only be used to setup things that would cause a boot failure
 * in the early boot process if they are not configured correctly.
 */
void z_arm_early_boot_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_EARLY_INIT_H_ */
