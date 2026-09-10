/*
 * Copyright (c) 2016-2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC32XX_SOC_H_
#define TI_SIMPLELINK_CC32XX_SOC_H_

#include <zephyr/arch/arm/cortex_m/nvic.h>

#include <inc/hw_types.h>
#include <driverlib/prcm.h>

/*
 * CMSIS IRQn_Type enum is broken relative to ARM GNU compiler.
 *
 * So redefine the IRQn_Type enum to a unsigned int to avoid
 * the ARM compiler from sign extending IRQn_Type values higher than 0x80
 * into negative IRQ values, which causes hard-to-debug Hard Faults.
 */
typedef uint32_t IRQn_Type;

/* Need to keep the remaining from cmsis.h, as Zephyr expects these. */
typedef enum {
	Reset_IRQn                    = -15,
	NonMaskableInt_IRQn           = -14,
	HardFault_IRQn                = -13,
	MemoryManagement_IRQn         = -12,
	BusFault_IRQn                 = -11,
	UsageFault_IRQn               = -10,
	SVCall_IRQn                   =  -5,
	DebugMonitor_IRQn             =  -4,
	PendSV_IRQn                   =  -2,
	SysTick_IRQn                  =  -1,
} CMSIS_IRQn_Type;

#define __CM4_REV        0
#define __MPU_PRESENT                  0 /* Zephyr has no MPU support */
#define __NVIC_PRIO_BITS               NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig         0 /* Default to standard SysTick */

#include <core_cm4.h>

#endif /* TI_SIMPLELINK_CC32XX_SOC_H_ */
