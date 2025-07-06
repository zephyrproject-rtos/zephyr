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

#ifndef ZEPHYR_MODULES_CMSIS_CMSIS_A_R_H_
#define ZEPHYR_MODULES_CMSIS_CMSIS_A_R_H_

#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __CR_REV
#define __CR_REV                0U
#endif

#ifndef __CA_REV
#define __CA_REV                0U
#endif

#ifndef __FPU_PRESENT
#define __FPU_PRESENT           CONFIG_CPU_HAS_FPU
#endif

#ifndef __MMU_PRESENT
#define __MMU_PRESENT           CONFIG_CPU_HAS_MMU
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
#elif defined(CONFIG_CPU_CORTEX_R8)
#include <core_cr8.h>
#elif defined(CONFIG_CPU_CORTEX_R52)
#include <core_cr52.h>
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A)
/*
 * Any defines relevant for the proper inclusion of CMSIS' Cortex-A
 * Common Peripheral Access Layer (such as __CORTEX_A) which are not
 * covered by the Kconfig-based default assignments above must be
 * provided by each aarch32 Cortex-A SoC's header file (already in-
 * cluded above).
 */
#include <core_ca.h>
#else
#error "Unknown device"
#endif

#include "cmsis_core_a_r_ext.h"

#endif /* ZEPHYR_MODULES_CMSIS_CMSIS_A_R_H_ */
