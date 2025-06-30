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

#ifndef ZEPHYR_ARCH_DSPIC_VECTOR_TABLE_H_
#define ZEPHYR_ARCH_DSPIC_VECTOR_TABLE_H_

#ifdef _ASMLANGUAGE

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>

#else /* _ASMLANGUAGE */

#ifdef __cplusplus
extern "C" {
#endif
extern void *_vector_table[CONFIG_NUM_IRQS];
#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_CORE_AARCH32_CORTEX_M_VECTOR_TABLE_H_ */
