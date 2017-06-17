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

#ifndef _VECTOR_TABLE__H_
#define _VECTOR_TABLE__H_

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
#if defined(CONFIG_ARMV6_M)
GTEXT(__svc)
#elif defined(CONFIG_ARMV7_M)
GTEXT(__mpu_fault)
GTEXT(__bus_fault)
GTEXT(__usage_fault)
GTEXT(__svc)
GTEXT(__debug_monitor)
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M */
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

#endif /* _VECTOR_TABLE__H_ */
