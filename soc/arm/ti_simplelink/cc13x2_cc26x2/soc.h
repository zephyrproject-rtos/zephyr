/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TI_SIMPLELINK_CC13X2_CC26X2_SOC_H_
#define TI_SIMPLELINK_CC13X2_CC26X2_SOC_H_

/* CMSIS required values */
typedef enum {
	Reset_IRQn            = -15,
	NonMaskableInt_IRQn   = -14,
	HardFault_IRQn        = -13,
	MemoryManagement_IRQn = -12,
	BusFault_IRQn         = -11,
	UsageFault_IRQn       = -10,
	SVCall_IRQn           =  -5,
	DebugMonitor_IRQn     =  -4,
	PendSV_IRQn           =  -2,
	SysTick_IRQn          =  -1,
} IRQn_Type;

#define __CM4_REV              0
#define __MPU_PRESENT          1
#define __NVIC_PRIO_BITS       DT_NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig 0
#define __FPU_PRESENT          1

#endif /* TI_SIMPLELINK_CC13X2_CC26X2_SOC_H_ */
