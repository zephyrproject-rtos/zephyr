/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _callee_saved {
	/* General purpose callee-saved registers */
	uint32_t sp; /* r3: stack pointer */
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {

	/* interrupt locking key */
	uint32_t key;

	/* r0 in stack frame cannot be written to reliably */
	uint32_t swap_return_value;
};

typedef struct _thread_arch _thread_arch_t;

#define ARCH_EXCEPT(reason_p)                                                                      \
	do {                                                                                       \
		arch_irq_unlock(0);                                                                \
		__asm__ volatile("mov %[_reason], r6\n\t"                                          \
				 "trap 0x10\n\t" ::[_reason] "r"(reason_p)                         \
				 : "r6", "memory");                                                \
	} while (false)

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_ */
