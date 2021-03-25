/*
 * Copyright (c) 2020, Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Management for Tightly Coupled Memory
 *
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_TCM_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_TCM_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else


#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Disable ECC on Tightly Coupled Memory Banks
 *
 * Notes:
 *
 * This function shall only be called in Privileged mode.
 *
 * @return N/A
 */
void z_arm_tcm_disable_ecc(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_A_R_TCM_H_ */
