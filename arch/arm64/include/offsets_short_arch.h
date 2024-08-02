/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

#define _thread_offset_to_exception_depth \
	(___thread_t_arch_OFFSET + ___thread_arch_t_exception_depth_OFFSET)

#define _thread_offset_to_callee_saved_x19_x20 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x19_x20_OFFSET)
#define _thread_offset_to_callee_saved_x21_x22 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x21_x22_OFFSET)
#define _thread_offset_to_callee_saved_x23_x24 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x23_x24_OFFSET)
#define _thread_offset_to_callee_saved_x25_x26 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x25_x26_OFFSET)
#define _thread_offset_to_callee_saved_x27_x28 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x27_x28_OFFSET)
#define _thread_offset_to_callee_saved_x29_sp_el0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_x29_sp_el0_OFFSET)
#define _thread_offset_to_callee_saved_sp_elx_lr \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_elx_lr_OFFSET)

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
#define _cpu_offset_to_safe_exception_stack \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_safe_exception_stack_OFFSET)
#define _cpu_offset_to_current_stack_limit \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_current_stack_limit_OFFSET)
#define _cpu_offset_to_corrupted_sp \
	(___cpu_t_arch_OFFSET + ___cpu_arch_t_corrupted_sp_OFFSET)
#define _thread_offset_to_stack_limit \
	(___thread_t_arch_OFFSET + ___thread_arch_t_stack_limit_OFFSET)
#endif

#endif /* ZEPHYR_ARCH_ARM64_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
