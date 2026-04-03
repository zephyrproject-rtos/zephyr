/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_MICROCHIP_MEC_MEC165XB_SOC_H
#define __SOC_MICROCHIP_MEC_MEC165XB_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(96)

#ifndef _ASMLANGUAGE

#define MCHP_HAS_UART_LSR2

/* Minimal ARM CMSIS requirements */
typedef enum {
	Reset_IRQn = -15,
	NonMaskableInt_IRQn = -14,
	HardFault_IRQn = -13,
	MemoryManagement_IRQn = -12,
	BusFault_IRQn = -11,
	UsageFault_IRQn = -10,
	SVCall_IRQn = -5,
	DebugMonitor_IRQn = -4,
	PendSV_IRQn = -2,
	SysTick_IRQn = -1,
	FirstPeriph_IRQn = 0,
	LastPeriph_IRQn = 197,
} IRQn_Type;

#define __CM4_REV              0x0201U /* CM4 Core Revision */
#define __NVIC_PRIO_BITS       3       /* Number of Bits used for Priority Levels */
#define __Vendor_SysTickConfig 0       /* Set to 1 if different SysTick Config is used */
#define __MPU_PRESENT          1       /* MPU present */
#define __FPU_PRESENT          0       /* FPU present */

#include <core_cm4.h>

/* common peripheral register defines */
#include <soc_common.h>

#endif
#endif
