/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS interface file
 *
 * This header populates the default values required to configure the
 * ARM CMSIS Core headers.
 */

#ifndef ZEPHYR_MODULES_CMSIS_CMSIS_M_DEFAULTS_H_
#define ZEPHYR_MODULES_CMSIS_CMSIS_M_DEFAULTS_H_

#include <zephyr/arch/arm/cortex_m/nvic.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fill in CMSIS required values for non-CMSIS compliant SoCs.
 * Use __NVIC_PRIO_BITS as it is required and simple to check, but
 * ultimately all SoCs will define their own CMSIS types and constants.
 */
#ifndef __NVIC_PRIO_BITS
typedef enum {
	Reset_IRQn                    = -15,
	NonMaskableInt_IRQn           = -14,
	HardFault_IRQn                = -13,
#if defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	MemoryManagement_IRQn         = -12,
	BusFault_IRQn                 = -11,
	UsageFault_IRQn               = -10,
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	SecureFault_IRQn              = -9,
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
#endif /* CONFIG_ARMV7_M_ARMV8_M_MAINLINE */
	SVCall_IRQn                   =  -5,
	DebugMonitor_IRQn             =  -4,
	PendSV_IRQn                   =  -2,
	SysTick_IRQn                  =  -1,
	Max_IRQn                      =  CONFIG_NUM_IRQS,
} IRQn_Type;

#if defined(CONFIG_CPU_CORTEX_M0)
#define __CM0_REV        0
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#define __CM0PLUS_REV    0
#elif defined(CONFIG_CPU_CORTEX_M1)
#define __CM1_REV        0
#elif defined(CONFIG_CPU_CORTEX_M3)
#define __CM3_REV        0
#elif defined(CONFIG_CPU_CORTEX_M4)
#define __CM4_REV        0
#elif defined(CONFIG_CPU_CORTEX_M7)
#define __CM7_REV        0
#elif defined(CONFIG_CPU_CORTEX_M23)
#define __CM23_REV       0
#elif defined(CONFIG_CPU_CORTEX_M33)
#define __CM33_REV       0
#elif defined(CONFIG_CPU_CORTEX_M55)
#define __CM55_REV       0
#else
#error "Unknown Cortex-M device"
#endif

#define __NVIC_PRIO_BITS               NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */
#endif /* __NVIC_PRIO_BITS */

#ifndef __MPU_PRESENT
#define __MPU_PRESENT             CONFIG_CPU_HAS_ARM_MPU
#endif

#ifndef __FPU_PRESENT
#define __FPU_PRESENT             CONFIG_CPU_HAS_FPU
#endif

#ifndef __FPU_DP
#define __FPU_DP                  CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION
#endif

#ifndef __VTOR_PRESENT
#define __VTOR_PRESENT            CONFIG_CPU_CORTEX_M_HAS_VTOR
#endif

#ifndef __DSP_PRESENT
#define __DSP_PRESENT             CONFIG_ARMV8_M_DSP
#endif

#ifndef __ICACHE_PRESENT
#define __ICACHE_PRESENT          CONFIG_CPU_HAS_ICACHE
#endif

#ifndef __DCACHE_PRESENT
#define __DCACHE_PRESENT          CONFIG_CPU_HAS_DCACHE
#endif

#ifndef __MVE_PRESENT
#define __MVE_PRESENT             CONFIG_ARMV8_1_M_MVEI
#endif

#ifndef __SAUREGION_PRESENT
#define __SAUREGION_PRESENT       CONFIG_CPU_HAS_ARM_SAU
#endif

#ifndef __PMU_PRESENT
#define __PMU_PRESENT             CONFIG_ARMV8_1_M_PMU
#define __PMU_NUM_EVENTCNT        CONFIG_ARMV8_1_M_PMU_EVENTCNT
#endif

#ifdef __cplusplus
}
#endif

#if defined(CONFIG_CPU_CORTEX_M0)
#include <core_cm0.h>
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#include <core_cm0plus.h>
#elif defined(CONFIG_CPU_CORTEX_M1)
#include <core_cm1.h>
#elif defined(CONFIG_CPU_CORTEX_M3)
#include <core_cm3.h>
#elif defined(CONFIG_CPU_CORTEX_M4)
#include <core_cm4.h>
#elif defined(CONFIG_CPU_CORTEX_M7)
#include <core_cm7.h>
#elif defined(CONFIG_CPU_CORTEX_M23)
#include <core_cm23.h>
#elif defined(CONFIG_CPU_CORTEX_M33)
#include <core_cm33.h>
#elif defined(CONFIG_CPU_CORTEX_M55)
#include <core_cm55.h>
#else
#error "Unknown Cortex-M device"
#endif

#endif /* ZEPHYR_MODULES_CMSIS_CMSIS_M_DEFAULTS_H_ */
