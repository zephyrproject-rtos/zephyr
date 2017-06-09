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
#define CPACR_CP10_Msk          (3UL << CPACR_CP10_Pos)
#define CPACR_CP10_NO_ACCESS    (0UL << CPACR_CP10_Pos)
#define CPACR_CP10_PRIV_ACCESS  (1UL << CPACR_CP10_Pos)
#define CPACR_CP10_RESERVED     (2UL << CPACR_CP10_Pos)
#define CPACR_CP10_FULL_ACCESS  (3UL << CPACR_CP10_Pos)

/* CP11 Access Bits */
#define CPACR_CP11_Pos          22U
#define CPACR_CP11_Msk          (3UL << CPACR_CP11_Pos)
#define CPACR_CP11_NO_ACCESS    (0UL << CPACR_CP11_Pos)
#define CPACR_CP11_PRIV_ACCESS  (1UL << CPACR_CP11_Pos)
#define CPACR_CP11_RESERVED     (2UL << CPACR_CP11_Pos)
#define CPACR_CP11_FULL_ACCESS  (3UL << CPACR_CP11_Pos)

#define SCB_UFSR  (*((__IOM u16_t *) &SCB->CFSR + 2))
#define SCB_BFSR  (*((__IOM u8_t *) &SCB->CFSR + 1))
#define SCB_MMFSR (*((__IOM u8_t *) &SCB->CFSR))

/* CFSR[UFSR] */
#define CFSR_DIVBYZERO_Pos		(25U)
#define CFSR_DIVBYZERO_Msk		(0x1U << CFSR_DIVBYZERO_Pos)
#define CFSR_UNALIGNED_Pos		(24U)
#define CFSR_UNALIGNED_Msk		(0x1U << CFSR_UNALIGNED_Pos)
#define CFSR_NOCP_Pos			(19U)
#define CFSR_NOCP_Msk			(0x1U << CFSR_NOCP_Pos)
#define CFSR_INVPC_Pos			(18U)
#define CFSR_INVPC_Msk			(0x1U << CFSR_INVPC_Pos)
#define CFSR_INVSTATE_Pos		(17U)
#define CFSR_INVSTATE_Msk		(0x1U << CFSR_INVSTATE_Pos)
#define CFSR_UNDEFINSTR_Pos		(16U)
#define CFSR_UNDEFINSTR_Msk		(0x1U << CFSR_UNDEFINSTR_Pos)

/* CFSR[BFSR] */
#define CFSR_BFARVALID_Pos		(15U)
#define CFSR_BFARVALID_Msk		(0x1U << CFSR_BFARVALID_Pos)
#define CFSR_LSPERR_Pos			(13U)
#define CFSR_LSPERR_Msk			(0x1U << CFSR_LSPERR_Pos)
#define CFSR_STKERR_Pos			(12U)
#define CFSR_STKERR_Msk			(0x1U << CFSR_STKERR_Pos)
#define CFSR_UNSTKERR_Pos		(11U)
#define CFSR_UNSTKERR_Msk		(0x1U << CFSR_UNSTKERR_Pos)
#define CFSR_IMPRECISERR_Pos		(10U)
#define CFSR_IMPRECISERR_Msk		(0x1U << CFSR_IMPRECISERR_Pos)
#define CFSR_PRECISERR_Pos		(9U)
#define CFSR_PRECISERR_Msk		(0x1U << CFSR_PRECISERR_Pos)
#define CFSR_IBUSERR_Pos		(8U)
#define CFSR_IBUSERR_Msk		(0x1U << CFSR_IBUSERR_Pos)

/* CFSR[MMFSR] */
#define CFSR_MMARVALID_Pos		(7U)
#define CFSR_MMARVALID_Msk		(0x1U << CFSR_MMARVALID_Pos)
#define CFSR_MLSPERR_Pos		(5U)
#define CFSR_MLSPERR_Msk		(0x1U << CFSR_MLSPERR_Pos)
#define CFSR_MSTKERR_Pos		(4U)
#define CFSR_MSTKERR_Msk		(0x1U << CFSR_MSTKERR_Pos)
#define CFSR_MUNSTKERR_Pos		(3U)
#define CFSR_MUNSTKERR_Msk		(0x1U << CFSR_MUNSTKERR_Pos)
#define CFSR_DACCVIOL_Pos		(1U)
#define CFSR_DACCVIOL_Msk		(0x1U << CFSR_DACCVIOL_Pos)
#define CFSR_IACCVIOL_Pos		(0U)
#define CFSR_IACCVIOL_Msk		(0x1U << CFSR_IACCVIOL_Pos)


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
#error "Unknown Cortex-M device"
#endif

#define __MPU_PRESENT                  0 /* Zephyr has no MPU support */
#define __NVIC_PRIO_BITS               CONFIG_NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */
#endif /* __NVIC_PRIO_BITS */

#if __NVIC_PRIO_BITS != CONFIG_NUM_IRQ_PRIO_BITS
#error "CONFIG_NUM_IRQ_PRIO_BITS and __NVIC_PRIO_BITS are not set to the same value"
#endif

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
#error "Unknown Cortex-M device"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _CMSIS__H_ */
