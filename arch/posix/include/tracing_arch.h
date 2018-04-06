/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for ARM
 */

#ifndef __KERNEL_TRACING_H__
#define __KERNEL_TRACING_H__

#include "posix_soc_if.h"

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
static inline int _sys_current_irq_key_get(void)
{
	return posix_get_current_irq();
}

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_TRACING_H__ */
