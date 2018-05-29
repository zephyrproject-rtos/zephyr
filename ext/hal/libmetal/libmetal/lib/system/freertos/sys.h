/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/sys.h
 * @brief	FreeRTOS system primitives for libmetal.
 */

#ifndef __METAL_SYS__H__
#error "Include metal/sys.h instead of metal/freertos/sys.h"
#endif

#ifndef __METAL_FREERTOS_SYS__H__
#define __METAL_FREERTOS_SYS__H__

#include "./@PROJECT_MACHINE@/sys.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef METAL_MAX_DEVICE_REGIONS
#define METAL_MAX_DEVICE_REGIONS 1
#endif

/** Structure for FreeRTOS libmetal runtime state. */
struct metal_state {

	/** Common (system independent) data. */
	struct metal_common_state common;
};

#ifdef METAL_INTERNAL

/**
 * @brief restore interrupts to state before disable_global_interrupt()
 */
void sys_irq_restore_enable(void);

/**
 * @brief disable all interrupts
 */
void sys_irq_save_disable(void);

#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_SYS__H__ */
