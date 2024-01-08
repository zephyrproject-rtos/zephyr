/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM AArch32 public exception handling
 *
 * ARM AArch32-specific kernel exception handling interface. Included by
 * arm/arch.h.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_EXC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_EXC_H_

#if defined(CONFIG_CPU_CORTEX_M)
#include <zephyr/arch/arm/cortex_m/exc.h>
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
#include <zephyr/arch/arm/cortex_a_r/exc.h>
#else
#error Unknown ARM architecture
#endif /* CONFIG_CPU_CORTEX_M */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_EXC_H_ */
