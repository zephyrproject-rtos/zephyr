/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_intlock_key \
	(___thread_t_arch_OFFSET + ___thread_arch_t_intlock_key_OFFSET)

#define _thread_offset_to_relinquish_cause \
	(___thread_t_arch_OFFSET + ___thread_arch_t_relinquish_cause_OFFSET)

#define _thread_offset_to_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_return_value_OFFSET)

#define _thread_offset_to_k_stack_base \
	(___thread_t_arch_OFFSET + ___thread_arch_t_k_stack_base_OFFSET)

#define _thread_offset_to_k_stack_top \
	(___thread_t_arch_OFFSET + ___thread_arch_t_k_stack_top_OFFSET)

#define _thread_offset_to_u_stack_base \
	(___thread_t_arch_OFFSET + ___thread_arch_t_u_stack_base_OFFSET)

#define _thread_offset_to_u_stack_top \
	(___thread_t_arch_OFFSET + ___thread_arch_t_u_stack_top_OFFSET)

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

/* end - threads */

#endif /* ZEPHYR_ARCH_ARC_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
