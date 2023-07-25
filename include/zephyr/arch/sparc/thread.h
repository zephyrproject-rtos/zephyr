/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
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

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	/* y register used by mul/div */
	uint32_t y;

	/* processor status register */
	uint32_t psr;

	/*
	 * local registers
	 *
	 * Using uint64_t l0_and_l1 will put everything in this structure on a
	 * double word boundary which allows us to use double word loads and
	 * stores safely in the context switch.
	 */
	uint64_t l0_and_l1;
	uint32_t l2;
	uint32_t l3;
	uint32_t l4;
	uint32_t l5;
	uint32_t l6;
	uint32_t l7;

	/* input registers */
	uint32_t i0;
	uint32_t i1;
	uint32_t i2;
	uint32_t i3;
	uint32_t i4;
	uint32_t i5;
	uint32_t i6; /* frame pointer */
	uint32_t i7;

	/* output registers */
	uint32_t o6; /* stack pointer */
	uint32_t o7;

};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* empty */
};

typedef struct _thread_arch _thread_arch_t;

#endif  /* _ASMLANGUAGE */

#endif  /* ZEPHYR_INCLUDE_ARCH_SPARC_THREAD_H_ */
