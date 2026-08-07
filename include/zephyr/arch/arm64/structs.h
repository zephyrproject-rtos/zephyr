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
	/* This struct will have a size 0 in C which is not allowed in C++ (it'll have a size 1). To
	 * prevent this, we add a 1 byte dummy variable.
	 */
	uint8_t dummy;
#endif
};

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_STRUCTS_H_ */
