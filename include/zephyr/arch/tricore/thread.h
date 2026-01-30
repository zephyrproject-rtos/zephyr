/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_TRICORE_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_TRICORE_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _callee_saved {
	uint32_t pcxi;
#ifdef CONFIG_CPU_TC18
	uint32_t pprs;
#endif
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	uint8_t prs;
};

typedef struct _thread_arch _thread_arch_t;

#define K_PROTECTION_SET_MASK GENMASK(7, 5)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_TREAD_THREAD_H_ */
