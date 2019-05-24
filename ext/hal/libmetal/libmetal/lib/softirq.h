/*
 * Copyright (c) 2019, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	softirq.h
 * @brief	Soft Interrupt handling primitives for libmetal.
 */

#ifndef __METAL_SOFTIRQ__H__
#define __METAL_SOFTIRQ__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup soft irq Interrupt Handling Interfaces
 *  @{ */

#include <metal/irq.h>

/**
 * @brief	metal_softirq_init
 *
 * Initialize libmetal soft IRQs controller
 *
 * @return 0 on success, or negative value for failure
 */
int metal_softirq_init();

/**
 * @brief	metal_softirq_dispatch
 *
 * Dispatch the pending soft IRQs
 */
void metal_softirq_dispatch();

/**
 * @brief	metal_softirq_allocate
 *
 * Allocate soft IRQs
 *
 * This function doesn't have any locking, it is not supposed
 * to be called by multiple threads.
 *
 * @param[in]  num number of soft irqs requested
 * @return soft irq base for success, or negative value for failure
 */
int metal_softirq_allocate(int num);

/**
 * @brief	metal_softirq_set
 *
 * Set soft IRQ to pending
 *
 * @param[in]  irq soft IRQ ID to set
 */
void metal_softirq_set(int irq);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_SOFTIRQ__H__ */
