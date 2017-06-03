/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inc/hw_types.h>
#include <driverlib/prcm.h>

/*
 * CMSIS IRQn_Type enum is broken relative to ARM GNU compiler.
 *
 * So redefine the IRQn_Type enum to a unsigned int to avoid
 * the ARM compiler from sign extending IRQn_Type values higher than 0x80
 * into negative IRQ values, which causes hard-to-debug Hard Faults.
 */
typedef u32_t IRQn_Type;

/* Need to keep the remaining from cmsis.h, as Zephyr expects these. */
enum {
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
} CMSIS_IRQn_Type;

#define __CM4_REV        0
#define __MPU_PRESENT                  0 /* Zephyr has no MPU support */
#define __NVIC_PRIO_BITS               CONFIG_NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */
