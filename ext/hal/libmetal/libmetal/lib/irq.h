/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	irq.h
 * @brief	Interrupt handling primitives for libmetal.
 */

#ifndef __METAL_IRQ__H__
#define __METAL_IRQ__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup irq Interrupt Handling Interfaces
 *  @{ */

#include <metal/list.h>
#include <stdlib.h>

/** IRQ handled status */
#define METAL_IRQ_NOT_HANDLED 0
#define METAL_IRQ_HANDLED     1

/**
 * @brief	type of interrupt handler
 * @param[in]	irq interrupt id
 * @param[in]	arg argument to pass to the handler
 * @return	irq handled status
 */
typedef int (*metal_irq_handler) (int irq, void *arg);

/**
 * @brief      Register interrupt handler for interrupt.
 *             Only allow single interrupt handler for a interrupt.
 *
 *             If irq_handler is NULL, it will unregister interrupt
 *             handler from interrupt
 *
 * @param[in]  irq         interrupt id
 * @param[in]  irq_handler interrupt handler
 * @param[in]  arg         arg is the argument pointing to the data which
 *                         will be passed to the interrupt handler.
 * @return     0 for success, non-zero on failure
 */
int metal_irq_register(int irq,
		       metal_irq_handler irq_handler,
		       void *arg);

/**
 * @brief      Unregister interrupt handler for interrupt.
 *
 * @param[in]  irq         interrupt id
 */
static inline
void metal_irq_unregister(int irq)
{
	metal_irq_register(irq, 0, NULL);
}

/**
 * @brief      disable interrupts
 * @return     interrupts state
 */
unsigned int metal_irq_save_disable(void);

/**
 * @brief      restore interrupts to their previous state
 * @param[in]  flags previous interrupts state
 */
void metal_irq_restore_enable(unsigned int flags);

/**
 * @brief	metal_irq_enable
 *
 * Enables the given interrupt
 *
 * @param vector   - interrupt vector number
 */
void metal_irq_enable(unsigned int vector);

/**
 * @brief	metal_irq_disable
 *
 * Disables the given interrupt
 *
 * @param vector   - interrupt vector number
 */
void metal_irq_disable(unsigned int vector);

#include <metal/system/@PROJECT_SYSTEM@/irq.h>

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_IRQ__H__ */
