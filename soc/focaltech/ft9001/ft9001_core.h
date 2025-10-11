/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __FT9001_CORE_H
#define __FT9001_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* IRQ numbers: define ONLY the core exceptions required by CMSIS. */
typedef enum IRQn {
	NonMaskableInt_IRQn = -14,
	HardFault_IRQn = -13,
	MemoryManagement_IRQn = -12,
	BusFault_IRQn = -11,
	UsageFault_IRQn = -10,
	SVCall_IRQn = -5,
	DebugMonitor_IRQn = -4,
	PendSV_IRQn = -2,
	SysTick_IRQn = -1
} IRQn_Type;

#ifndef __NVIC_PRIO_BITS
#include <zephyr/arch/arm/cortex_m/nvic.h>
#ifndef NUM_IRQ_PRIO_BITS
#define NUM_IRQ_PRIO_BITS 3
#endif
#define __NVIC_PRIO_BITS NUM_IRQ_PRIO_BITS
#endif

#ifndef __MPU_PRESENT
#ifdef CONFIG_ARM_MPU
#define __MPU_PRESENT 1
#else
#define __MPU_PRESENT 0
#endif
#endif

#ifndef __FPU_PRESENT
#ifdef CONFIG_CPU_HAS_FPU
#define __FPU_PRESENT 1
#else
#define __FPU_PRESENT 0
#endif
#endif

#include "core_cm4.h"

#ifdef __cplusplus
}
#endif

#endif /* __FT9001_CORE_H */
