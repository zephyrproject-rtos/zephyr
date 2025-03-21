/*
 * Copyright (c) 2025 Intel
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_X86_STRUCTS_H_
#define ZEPHYR_INCLUDE_X86_STRUCTS_H_

#include <stdint.h>

struct k_thread;

/* Per CPU architecture specifics (empty) */
struct _cpu_arch {

#if defined(CONFIG_FPU_SHARING)
	/*
	 * A 'sse_owner' field does not exist in addition to the 'fpu_owner'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers into 2 distinct blocks owned by differing threads.  In
	 * other words, given that the 'fxnsave/fxrstor' instructions
	 * save/restore both the X87 FPU and XMM registers, it's not possible
	 * for a thread to only "own" the XMM registers.
	 */

	struct k_thread *fpu_owner;
#elif defined(__cplusplus)
	/* Ensure this struct does not have a size of 0 which is not allowed in C++. */
	uint8_t dummy;
#endif
};

#endif /* ZEPHYR_INCLUDE_X86_STRUCTS_H_ */
