/*
 * Copyright (c) BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARM64_STRUCTS_H_
#define ZEPHYR_INCLUDE_ARM64_STRUCTS_H_

/* Per CPU architecture specifics */
struct _cpu_arch {
#ifdef CONFIG_FPU_SHARING
	struct k_thread *fpu_owner;
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	uint64_t safe_exception_stack;
	uint64_t current_stack_limit;
	/* Saved the corrupted stack pointer when stack overflow, else 0 */
	uint64_t corrupted_sp;
#endif
};

#endif /* ZEPHYR_INCLUDE_ARM64_STRUCTS_H_ */
