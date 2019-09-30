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

#ifdef _ASMLANGUAGE

#include <toolchain.h>
#include <linker/sections.h>
#include <sys/util.h>

GTEXT(__start)
GTEXT(_vector_table)

GTEXT(z_arm_reset)
GTEXT(z_arm_nmi)
GTEXT(z_arm_hard_fault)
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
GTEXT(z_arm_svc)
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
GTEXT(z_arm_mpu_fault)
GTEXT(z_arm_bus_fault)
GTEXT(z_arm_usage_fault)
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
GTEXT(z_arm_secure_fault)
#endif /* CONFIG_ARM_SECURE_FIRMWARE */
GTEXT(z_arm_svc)
GTEXT(z_arm_debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
GTEXT(z_arm_pendsv)
GTEXT(z_arm_reserved)

GTEXT(z_arm_prep_c)
GTEXT(_isr_wrapper)

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif

extern void *_vector_table[];

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_CORE_CORTEX_M_VECTOR_TABLE_H_ */
