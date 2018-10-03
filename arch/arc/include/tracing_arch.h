/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for ARM
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_TRACING_ARCH_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_TRACING_ARCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the identification of the current interrupt.
 *
 * This routine obtain the key of the interrupt that is currently processed
 * if it is called from a ISR context.
 *
 * @return The key of the interrupt that is currently being processed.
 */
int _sys_current_irq_key_get(void)
{
	return _INTERRUPT_CAUSE();
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_TRACING_ARCH_H_ */
