/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support for Nios II
 */

#ifndef ZEPHYR_ARCH_NIOS2_INCLUDE_TRACING_ARCH_H_
#define ZEPHYR_ARCH_NIOS2_INCLUDE_TRACING_ARCH_H_

#include <arch/cpu.h>

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
	u32_t ipending;

	ipending = _nios2_creg_read(NIOS2_CR_IPENDING);
	return find_lsb_set(ipending) - 1;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_NIOS2_INCLUDE_TRACING_ARCH_H_ */
