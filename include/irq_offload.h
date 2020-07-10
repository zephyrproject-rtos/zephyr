/*
 * Copyright (c) 2015 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IRQ Offload interface
 */
#ifndef ZEPHYR_INCLUDE_IRQ_OFFLOAD_H_
#define ZEPHYR_INCLUDE_IRQ_OFFLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_IRQ_OFFLOAD
	typedef void (*irq_offload_routine_t)(const void *parameter);

/**
 * @brief Run a function in interrupt context
 *
 * This function synchronously runs the provided function in interrupt
 * context, passing in the supplied device. Useful for test code
 * which needs to show that kernel objects work correctly in interrupt
 * context.
 *
 * @param routine The function to run
 * @param parameter Argument to pass to the function when it is run as an
 * interrupt
 */
void irq_offload(irq_offload_routine_t routine, const void *parameter);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SW_IRQ_H_ */
