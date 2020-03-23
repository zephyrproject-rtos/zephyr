/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stack helpers for AArch64 Cortex-A CPUs
 *
 * Stack helper functions.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_STACK_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_STACK_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#ifdef __cplusplus
extern "C" {
#endif

struct _callee_saved_stack {
	uint64_t x29, x30;
	uint64_t x27, x28;
	uint64_t x25, x26;
	uint64_t x23, x24;
	uint64_t x21, x22;
	uint64_t x19, x20;
};

typedef struct _callee_saved_stack _callee_saved_stack_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH64_STACK_H_ */
