/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_H_

#include "xtensa-asm2-context.h"

/**
 * Initializes a stack area such that it can be "restored" later and
 * begin running with the specified function and three arguments.  The
 * entry function takes three arguments to match the signature of
 * Zephyr's k_thread_entry_t.  Thread will start with EXCM clear and
 * INTLEVEL set to zero (i.e. it's a user thread, we don't start with
 * anything masked, so don't assume that!).
 */
void *xtensa_init_stack(int *stack_top,
			void (*entry)(void *, void *, void *),
			void *arg1, void *arg2, void *arg3);

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_H_ */
