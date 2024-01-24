/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_
#define ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_

#include <zephyr/offsets.h>
#include <offsets_short_arch.h>

/* kernel */

/* main */
#ifndef CONFIG_SMP
/* Relies on _kernel.cpu being the first member of _kernel and having 1 element
 */
#define _kernel_offset_to_nested \
	(___cpu_t_nested_OFFSET)

#define _kernel_offset_to_irq_stack \
	(___cpu_t_irq_stack_OFFSET)

#define _kernel_offset_to_current \
	(___cpu_t_current_OFFSET)

#if defined(CONFIG_FPU_SHARING)
#define _kernel_offset_to_fp_ctx \
	(___cpu_t_fp_ctx_OFFSET)
#endif /* CONFIG_FPU_SHARING */
#endif /* CONFIG_SMP */

#define _kernel_offset_to_idle \
	(___kernel_t_idle_OFFSET)

#define _kernel_offset_to_current_fp \
	(___kernel_t_current_fp_OFFSET)

#define _kernel_offset_to_ready_q_cache \
	(___kernel_t_ready_q_OFFSET + ___ready_q_t_cache_OFFSET)

/* end - kernel */

/* threads */

/* main */

#define _thread_offset_to_callee_saved \
	(___thread_t_callee_saved_OFFSET)

#ifdef CONFIG_THREAD_LOCAL_STORAGE
#define _thread_offset_to_tls \
	(___thread_t_tls_OFFSET)
#endif /* CONFIG_THREAD_LOCAL_STORAGE */

/* base */

#define _thread_offset_to_user_options \
	(___thread_t_base_OFFSET + ___thread_base_t_user_options_OFFSET)

/* end - threads */

#endif /* ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_ */
