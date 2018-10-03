/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *  struct _caller_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	u32_t sp;       /* Stack pointer, (x2 register) */

	u32_t s0;       /* saved register/frame pointer */
	u32_t s1;       /* saved register */
	u32_t s2;       /* saved register */
	u32_t s3;       /* saved register */
	u32_t s4;       /* saved register */
	u32_t s5;       /* saved register */
	u32_t s6;       /* saved register */
	u32_t s7;       /* saved register */
	u32_t s8;       /* saved register */
	u32_t s9;       /* saved register */
	u32_t s10;      /* saved register */
	u32_t s11;      /* saved register */
};
typedef struct _callee_saved _callee_saved_t;

struct _caller_saved {
	/*
	 * Nothing here, the exception code puts all the caller-saved
	 * registers onto the stack.
	 */
};

typedef struct _caller_saved _caller_saved_t;

struct _thread_arch {
	u32_t swap_return_value; /* Return value of _Swap() */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RISCV32_INCLUDE_KERNEL_ARCH_THREAD_H_ */

