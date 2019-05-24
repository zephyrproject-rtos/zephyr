/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	irq.h
 * @brief	Interrupt handling primitives for libmetal.
 */

#ifndef __METAL_IRQ_CONTROLLER__H__
#define __METAL_IRQ_CONTROLLER__H__

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup irq Interrupt Handling Interfaces
 *  @{ */

#include <metal/irq.h>
#include <metal/list.h>
#include <stdlib.h>

/** IRQ ANY ID */
#define METAL_IRQ_ANY         (-1)

/** IRQ state macro which will be passed to metal irq set state function
 *  to indicate which state the caller want the IRQ to change to.
 */
#define METAL_IRQ_DISABLE    0U
#define METAL_IRQ_ENABLE     1U

struct metal_irq_controller;

/**
 * @brief	type of interrupt controller to set irq enable
 * @param[in]   irq_cntr pointer to interrupt controller
 * @param[in]   irq interrupt id
 * @param[in]   enable IRQ state
 */
typedef void (*metal_irq_set_enable) (struct metal_irq_controller *irq_cntr,
				     int irq, unsigned int enable);

/**
 * @brief	type of controller specific registering interrupt function
 * @param[in]   irq_cntr pointer to interrupt controller
 * @param[in]   irq interrupt id
 * @param[in]   hd interrupt handler
 * @param[in]   arg argument which will be passed to the interrupt handler
 * @return 0 for success, negative value for failure
 */
typedef int (*metal_cntr_irq_register) (struct metal_irq_controller *irq_cntr,
					int irq, metal_irq_handler hd,
					void *arg);

/** Libmetal interrupt structure */
struct metal_irq {
	metal_irq_handler hd; /**< Interrupt handler */
	void *arg; /**< Argument to pass to the interrupt handler */
};

/** Libmetal interrupt controller structure */
struct metal_irq_controller {
	int irq_base; /**< Start of IRQ number of the range managed by
		           the IRQ controller */
	int irq_num; /**< Number of IRQs managed by the IRQ controller */
	void *arg; /**< Argument to pass to interrupt controller function */
	metal_irq_set_enable irq_set_enable; /**< function to set IRQ eanble */
	metal_cntr_irq_register irq_register; /**< function to register IRQ
						   handler */
	struct metal_list node; /**< list node */
	struct metal_irq *irqs; /**< Array of IRQs managed by the controller */
};

#define METAL_IRQ_CONTROLLER_DECLARE(_irq_controller, \
				     _irq_base, _irq_num, \
				     _arg, \
				     _irq_set_enable, \
				     _irq_register, \
				     _irqs) \
	struct metal_irq_controller _irq_controller = { \
		.irq_base = _irq_base, \
		.irq_num = _irq_num, \
		.arg = _arg, \
		.irq_set_enable = _irq_set_enable, \
		.irq_register = _irq_register, \
		.irqs = _irqs,\
	};

/**
 * @brief	metal_irq_register_controller
 *
 * Register IRQ controller
 * This function will allocate IRQ ids if it was
 * not predefined in the irq controller. There is no
 * locking in the function, it is not supposed to be
 * called by multiple threads.
 *
 * @param[in] cntr  Interrupt controller to register
 * @return 0 on success, or negative value for failure.
 */
int metal_irq_register_controller(struct metal_irq_controller *cntr);

/**
 * @brief	metal_irq_handle
 *
 * Call registered IRQ handler
 *
 * @param[in] irq_data metal IRQ structure
 * @param[in] irq IRQ id which will be passed to handler
 * @return IRQ handler status
 */
static inline
int metal_irq_handle(struct metal_irq *irq_data, int irq)
{
	if (irq_data != NULL && irq_data->hd != NULL) {
		return irq_data->hd(irq, irq_data->arg);
	} else {
		return METAL_IRQ_NOT_HANDLED;
	}
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_IRQ__H__ */
