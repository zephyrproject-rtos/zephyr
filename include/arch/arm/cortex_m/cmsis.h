/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CMSIS interface file
 *
 * This header contains the interface to the ARM CMSIS Core headers.
 */

#ifndef _CMSIS__H_
#define _CMSIS__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <soc.h>

/* CP10 Access Bits */
#define CPACR_CP10_Pos          20U
#define CPACR_CP10_Msk          (3UL << _SCS_CPACR_CP10_Pos)
#define CPACR_CP10_NO_ACCESS    (0UL << _SCS_CPACR_CP10_Pos)
#define CPACR_CP10_PRIV_ACCESS  (1UL << _SCS_CPACR_CP10_Pos)
#define CPACR_CP10_RESERVED     (2UL << _SCS_CPACR_CP10_Pos)
#define CPACR_CP10_FULL_ACCESS  (3UL << _SCS_CPACR_CP10_Pos)

/* CP11 Access Bits */
#define CPACR_CP11_Pos          22U
#define CPACR_CP11_Msk          (3UL << _SCS_CPACR_CP11_Pos)
#define CPACR_CP11_NO_ACCESS    (0UL << _SCS_CPACR_CP11_Pos)
#define CPACR_CP11_PRIV_ACCESS  (1UL << _SCS_CPACR_CP11_Pos)
#define CPACR_CP11_RESERVED     (2UL << _SCS_CPACR_CP11_Pos)
#define CPACR_CP11_FULL_ACCESS  (3UL << _SCS_CPACR_CP11_Pos)

/* Fill in CMSIS required values for non-CMSIS compliant SoCs.
 * Use __NVIC_PRIO_BITS as it is required and simple to check, but
 * ultimately all SoCs will define their own CMSIS types and constants.
 */
#ifndef __NVIC_PRIO_BITS
typedef enum {
	Reset_IRQn                    = -15,
	NonMaskableInt_IRQn           = -14,
	HardFault_IRQn                = -13,
#if defined(CONFIG_ARMV7_M)
	MemoryManagement_IRQn         = -12,
	BusFault_IRQn                 = -11,
	UsageFault_IRQn               = -10,
#endif /* CONFIG_ARMV7_M */
	SVCall_IRQn                   =  -5,
	DebugMonitor_IRQn             =  -4,
	PendSV_IRQn                   =  -2,
	SysTick_IRQn                  =  -1,
} IRQn_Type;

#if defined(CONFIG_CPU_CORTEX_M0)
#define __CM0_REV        0
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#define __CM0PLUS_REV    0
#elif defined(CONFIG_CPU_CORTEX_M3)
#define __CM3_REV        0
#elif defined(CONFIG_CPU_CORTEX_M4)
#define __CM4_REV        0
#elif defined(CONFIG_CPU_CORTEX_M7)
#define __CM7_REV        0
#else
#error "Uknown Cortex-M device"
#endif

#define __MPU_PRESENT                  0 /* Zephyr has no MPU support */
#define __NVIC_PRIO_BITS               CONFIG_NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */
#endif /* __NVIC_PRIO_BITS */

#if defined(CONFIG_CPU_CORTEX_M0)
#include <core_cm0.h>
#elif defined(CONFIG_CPU_CORTEX_M0PLUS)
#include <core_cm0plus.h>
#elif defined(CONFIG_CPU_CORTEX_M3)
#include <core_cm3.h>
#elif defined(CONFIG_CPU_CORTEX_M4)
#include <core_cm4.h>
#elif defined(CONFIG_CPU_CORTEX_M7)
#include <core_cm7.h>
#else
#error "Uknown Cortex-M device"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _CMSIS__H_ */
