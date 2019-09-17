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
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_THREAD_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	ulong_t sp;	/* Stack pointer, (x2 register) */

	ulong_t s0;	/* saved register/frame pointer */
	ulong_t s1;	/* saved register */
	ulong_t s2;	/* saved register */
	ulong_t s3;	/* saved register */
	ulong_t s4;	/* saved register */
	ulong_t s5;	/* saved register */
	ulong_t s6;	/* saved register */
	ulong_t s7;	/* saved register */
	ulong_t s8;	/* saved register */
	ulong_t s9;	/* saved register */
	ulong_t s10;	/* saved register */
	ulong_t s11;	/* saved register */

#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)

	u32_t fcsr;	/* always 32bit width */
#if defined(CONFIG_FLOAT_64BIT)
	u64_t fs0;	/* saved floating register */
	u64_t fs1;	/* saved floating register */
	u64_t fs2;	/* saved floating register */
	u64_t fs3;	/* saved floating register */
	u64_t fs4;	/* saved floating register */
	u64_t fs5;	/* saved floating register */
	u64_t fs6;	/* saved floating register */
	u64_t fs7;	/* saved floating register */
	u64_t fs8;	/* saved floating register */
	u64_t fs9;	/* saved floating register */
	u64_t fs10;	/* saved floating register */
	u64_t fs11;	/* saved floating register */
#else
	u32_t fs0;	/* saved floating register */
	u32_t fs1;	/* saved floating register */
	u32_t fs2;	/* saved floating register */
	u32_t fs3;	/* saved floating register */
	u32_t fs4;	/* saved floating register */
	u32_t fs5;	/* saved floating register */
	u32_t fs6;	/* saved floating register */
	u32_t fs7;	/* saved floating register */
	u32_t fs8;	/* saved floating register */
	u32_t fs9;	/* saved floating register */
	u32_t fs10;	/* saved floating register */
	u32_t fs11;	/* saved floating register */
#endif	/* CONFIG_FLOAT_64BIT */

#endif
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	u32_t swap_return_value; /* Return value of z_swap() */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_KERNEL_ARCH_THREAD_H_ */
