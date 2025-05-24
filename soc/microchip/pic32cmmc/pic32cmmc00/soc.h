/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CMMC00_SOC_H_
#define _SOC_MICROCHIP_PIC32CMMC00_SOC_H_

#include <zephyr/arch/arm/cortex_m/nvic.h>

#define __NVIC_PRIO_BITS NUM_IRQ_PRIO_BITS
#define __VTOR_PRESENT   CONFIG_CPU_CORTEX_M_HAS_VTOR

typedef enum IRQn {
	PendSV_IRQn = -2,
	SysTick_IRQn = -1,
} IRQn_Type;

#include <core_cm0plus.h>

#endif /* _SOC_MICROCHIP_PIC32CMMC00_SOC_H_ */
