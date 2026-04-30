/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief dsPIC33A architecture inline functions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>

/**
 * @brief Return the number of CPUs in the system.
 *
 * @return Number of CPUs configured via CONFIG_MP_MAX_NUM_CPUS
 */
static ALWAYS_INLINE unsigned int arch_num_cpus(void)
{
	return CONFIG_MP_MAX_NUM_CPUS;
}

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_ARCH_INLINES_H */
