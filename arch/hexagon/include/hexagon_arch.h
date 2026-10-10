/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_ARCH_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_ARCH_H_

/**
 * @file
 * @brief Hexagon control register access for stack protection
 */

#ifndef _ASMLANGUAGE
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void hexagon_set_framelimit(uintptr_t limit)
{
	__asm__ volatile("c16 = %[limit]" : : [limit] "r"(limit) : "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_HEXAGON_ARCH_H_ */
