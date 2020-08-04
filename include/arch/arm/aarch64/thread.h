/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
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

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

struct _callee_saved {
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29; /* FP */
	uint64_t x30; /* LR */
	uint64_t sp;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* empty */
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH64_THREAD_H_ */
