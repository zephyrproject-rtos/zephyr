/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_
#define ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_

#include <offsets.h>
#include <offsets_short_arch.h>

/* kernel */

/* main */

#define _kernel_offset_to_nested \
	(___kernel_t_nested_OFFSET)

#define _kernel_offset_to_irq_stack \
	(___kernel_t_irq_stack_OFFSET)

#define _kernel_offset_to_current \
	(___kernel_t_current_OFFSET)

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

/* base */

#define _thread_offset_to_thread_state \
	(___thread_t_base_OFFSET + ___thread_base_t_thread_state_OFFSET)

#define _thread_offset_to_user_options \
	(___thread_t_base_OFFSET + ___thread_base_t_user_options_OFFSET)

#define _thread_offset_to_prio \
	(___thread_t_base_OFFSET + ___thread_base_t_prio_OFFSET)

#define _thread_offset_to_sched_locked \
	(___thread_t_base_OFFSET + ___thread_base_t_sched_locked_OFFSET)

#define _thread_offset_to_preempt \
	(___thread_t_base_OFFSET + ___thread_base_t_preempt_OFFSET)

#define _thread_offset_to_esf \
	(___thread_t_arch_OFFSET + ___thread_arch_t_esf_OFFSET)

#define _thread_offset_to_stack_start \
	(___thread_t_stack_info_OFFSET + ___thread_stack_info_t_start_OFFSET)
/* end - threads */

#endif /* ZEPHYR_KERNEL_INCLUDE_OFFSETS_SHORT_H_ */
