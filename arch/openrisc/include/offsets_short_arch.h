/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_OPENRISC_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_OPENRISC_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_r1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r1_OFFSET)

#define _thread_offset_to_r2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r2_OFFSET)

#define _thread_offset_to_r9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r9_OFFSET)

#define _thread_offset_to_r10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r10_OFFSET)

#define _thread_offset_to_r14 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r14_OFFSET)

#define _thread_offset_to_r16 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r16_OFFSET)

#define _thread_offset_to_r18 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r18_OFFSET)

#define _thread_offset_to_r20 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r20_OFFSET)

#define _thread_offset_to_r22 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r22_OFFSET)

#define _thread_offset_to_r24 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r24_OFFSET)

#define _thread_offset_to_r26 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r26_OFFSET)

#define _thread_offset_to_r28 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r28_OFFSET)

#define _thread_offset_to_r30 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r30_OFFSET)

#endif /* ZEPHYR_ARCH_OPENRISC_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
