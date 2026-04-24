/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_TRICORE_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_TRICORE_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_thread_state \
	(___thread_t_base_OFFSET + ___thread_base_t_thread_state_OFFSET)

#define _thread_offset_to_entry \
	(___thread_t_arch_OFFSET + ___thread_arch_t_entry_OFFSET)

#define _thread_offset_to_p1 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_p1_OFFSET)

#define _thread_offset_to_p2 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_p2_OFFSET)

#define _thread_offset_to_p3 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_p3_OFFSET)

#define _thread_offset_to_a10 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_a10_OFFSET)

#define _thread_offset_to_a11 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_a11_OFFSET)

#define _thread_offset_to_psw \
	(___thread_t_arch_OFFSET + ___thread_arch_t_psw_OFFSET)


#define _thread_offset_to_saved_a11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_a11_OFFSET)

#define _thread_offset_to_saved_pcxi \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_pcxi_OFFSET)

#endif

