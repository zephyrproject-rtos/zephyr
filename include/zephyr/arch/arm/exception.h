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

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_

#if defined(CONFIG_CPU_CORTEX_M)
#include <zephyr/arch/arm/cortex_m/exception.h>
#elif defined(CONFIG_CPU_AARCH32_CORTEX_A) || defined(CONFIG_CPU_AARCH32_CORTEX_R)
#include <zephyr/arch/arm/cortex_a_r/exception.h>
#else
#error Unknown ARM architecture
#endif /* CONFIG_CPU_CORTEX_M */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_EXCEPTION_H_ */
