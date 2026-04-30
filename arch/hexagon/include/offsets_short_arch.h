/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* Shortened offset macros for assembly code */
#define _thread_offset_to_sp (___thread_t_callee_saved_OFFSET + ___callee_saved_t_r29_sp_OFFSET)

#define _thread_offset_to_lr (___thread_t_callee_saved_OFFSET + ___callee_saved_t_r31_lr_OFFSET)

#define _thread_offset_to_fp (___thread_t_callee_saved_OFFSET + ___callee_saved_t_r30_fp_OFFSET)

#define _thread_offset_to_swap_return_value                                                        \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
