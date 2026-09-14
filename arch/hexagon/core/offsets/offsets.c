/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/exception.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_arch_t, swap_return_value);

GEN_OFFSET_SYM(_callee_saved_t, r29_sp);
GEN_OFFSET_SYM(_callee_saved_t, r30_fp);
GEN_OFFSET_SYM(_callee_saved_t, r31_lr);

GEN_ABS_SYM_END
