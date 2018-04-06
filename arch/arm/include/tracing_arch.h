/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for ARM
 */

#ifndef __KERNEL_TRACING_H__
#define __KERNEL_TRACING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "arch/arm/cortex_m/cmsis.h"
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
	return __get_IPSR();
}

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_TRACING_H__ */
