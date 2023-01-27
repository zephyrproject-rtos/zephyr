/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
 * Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef ZEPHYR_ARCH_MICROBLAZE_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_MICROBLAZE_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

#define _thread_offset_to_r1 (___thread_t_callee_saved_OFFSET + ___callee_saved_t_r1_OFFSET)

#define _thread_offset_to_key (___thread_t_callee_saved_OFFSET + ___callee_saved_t_key_OFFSET)

#define _thread_offset_to_retval (___thread_t_callee_saved_OFFSET + ___callee_saved_t_retval_OFFSET)

#define _thread_offset_to_preempted                                                                \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_preempted_OFFSET)

#endif /* ZEPHYR_ARCH_MICROBLAZE_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
