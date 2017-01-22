/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for Xtensa
 */

#ifndef __KERNEL_EVENT_LOGGER_ARCH_H__
#define __KERNEL_EVENT_LOGGER_ARCH_H__


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
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __KERNEL_EVENT_LOGGER_ARCH_H__ */
