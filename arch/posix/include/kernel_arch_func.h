/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_

#include "kernel.h"
#include <toolchain/common.h>
#include "posix_core.h"

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN)
void _arch_switch_to_main_thread(struct k_thread *main_thread,
		k_thread_stack_t *main_stack,
		size_t main_stack_size, k_thread_entry_t _main);
#endif

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the kernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
static inline void kernel_arch_init(void)
{
	/* Nothing to be done */
}



static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->callee_saved.retval = value;
}


/*
 * _IntLibInit() is called from the non-arch specific function,
 * prepare_multithreading().
 */
static inline void _IntLibInit(void)
{
	posix_init_multithreading();
}

#ifdef __cplusplus
}
#endif

#define _is_in_isr() (_kernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_POSIX_INCLUDE_KERNEL_ARCH_FUNC_H_ */
