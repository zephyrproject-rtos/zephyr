/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS interface file
 *
 * This header contains the interface to the ARM CMSIS Core headers.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CMSIS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CMSIS_H_

#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __CR_REV
#define __CR_REV                0U
#endif

#ifndef __FPU_PRESENT
#define __FPU_PRESENT           CONFIG_CPU_HAS_FPU
#endif

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_CPU_CORTEX_R4)
#include <core_cr4.h>
#elif defined(CONFIG_CPU_CORTEX_R5)
#include <core_cr5.h>
#elif defined(CONFIG_CPU_CORTEX_R7)
#include <core_cr7.h>
#else
#error "Unknown device"
#endif

#include <arch/arm/aarch32/cortex_a_r/cmsis_ext.h>

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_CMSIS_H_ */
