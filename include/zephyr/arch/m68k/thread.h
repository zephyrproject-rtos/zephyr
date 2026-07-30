/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_M68K_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_M68K_THREAD_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/* MOVEM keeps the register image on the stack; only its SP is stored here. */
struct _callee_saved {
	uintptr_t sp;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_M68K_THREAD_H_ */
