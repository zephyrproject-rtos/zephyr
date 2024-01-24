/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_NIOS2_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_NIOS2_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_r16 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r16_OFFSET)

#define _thread_offset_to_r17 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r17_OFFSET)

#define _thread_offset_to_r18 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r18_OFFSET)

#define _thread_offset_to_r19 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r19_OFFSET)

#define _thread_offset_to_r20 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r20_OFFSET)

#define _thread_offset_to_r21 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r21_OFFSET)

#define _thread_offset_to_r22 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r22_OFFSET)

#define _thread_offset_to_r23 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r23_OFFSET)

#define _thread_offset_to_r28 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r28_OFFSET)

#define _thread_offset_to_ra \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_ra_OFFSET)

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

#define _thread_offset_to_key \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_key_OFFSET)

#define _thread_offset_to_retval \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

/* end - threads */

#endif /* ZEPHYR_ARCH_NIOS2_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
