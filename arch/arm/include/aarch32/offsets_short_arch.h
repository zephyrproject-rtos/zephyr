/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_basepri \
	(___thread_t_arch_OFFSET + ___thread_arch_t_basepri_OFFSET)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#define _thread_offset_to_preempt_float \
	(___thread_t_arch_OFFSET + ___thread_arch_t_preempt_float_OFFSET)

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FPU_SHARING)
#define _thread_offset_to_mode \
	(___thread_t_arch_OFFSET + ___thread_arch_t_mode_OFFSET)
#endif

#if defined(CONFIG_ARM_STORE_EXC_RETURN)
#define _thread_offset_to_mode_exc_return \
	(___thread_t_arch_OFFSET + ___thread_arch_t_mode_exc_return_OFFSET)
#endif

#ifdef CONFIG_USERSPACE
#define _thread_offset_to_priv_stack_start \
	(___thread_t_arch_OFFSET + ___thread_arch_t_priv_stack_start_OFFSET)
#endif

#if defined(CONFIG_THREAD_STACK_INFO)
#define _thread_offset_to_stack_info_start \
	(___thread_stack_info_t_start_OFFSET + ___thread_t_stack_info_OFFSET)
#endif


/* end - threads */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_OFFSETS_SHORT_ARCH_H_ */
