/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_FPU_SHARING
	atomic_ptr_val_t fpu_owner;
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	uint64_t safe_exception_stack;
	uint64_t current_stack_limit;
	/* Saved the corrupted stack pointer when stack overflow, else 0 */
	uint64_t corrupted_sp;
#endif
#if !defined(CONFIG_FPU_SHARING) && !defined(CONFIG_ARM64_SAFE_EXCEPTION_STACK) &&                 \
	defined(__cplusplus)
	/* An empty struct is not valid C, and compilers that accept it give
	 * it size 0 while C++ gives 1. Keep a byte so both languages agree.
	 */
	uint8_t dummy;
#endif
};

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_STRUCTS_H_ */
