/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions for the boot vector table
 *
 *
 * Definitions for the boot vector table.
 *
 * System exception handler names all have the same format:
 *
 *   __<exception name with underscores>
 *
 * No other symbol has the same format, so they are easy to spot.
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_CORTEX_M_VECTOR_TABLE_H_
#define ZEPHYR_ARCH_ARM_CORE_CORTEX_M_VECTOR_TABLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

#include <board.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <misc/util.h>

GTEXT(__start)
GTEXT(_vector_table)

GTEXT(__reset)
GTEXT(__nmi)
GTEXT(__hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
GTEXT(__svc)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(__secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
GTEXT(__svc)
GTEXT(__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(__pendsv)
GTEXT(__reserved)

GTEXT(_PrepC)
GTEXT(_isr_wrapper)

#else

extern void *_vector_table[];

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_CORE_CORTEX_M_VECTOR_TABLE_H_ */
