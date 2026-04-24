/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>

/* Kernel offsets */
#include <kernel_offsets.h>

/* struct coop member offsets */
GEN_OFFSET_SYM(_callee_saved_t, pcxi);

GEN_OFFSET_SYM(_thread_base_t, thread_state);

/* CSA offsets */
GEN_OFFSET_SYM(z_tricore_lower_context_t, a4);
GEN_OFFSET_SYM(z_tricore_lower_context_t, a5);
GEN_OFFSET_SYM(z_tricore_lower_context_t, a6);
GEN_OFFSET_SYM(z_tricore_lower_context_t, a7);
GEN_OFFSET_SYM(z_tricore_upper_context_t, a10);
GEN_OFFSET_SYM(z_tricore_lower_context_t, a11);
GEN_OFFSET_SYM(z_tricore_upper_context_t, psw);
GEN_OFFSET_SYM(z_tricore_upper_context_t, pcxi);

GEN_ABSOLUTE_SYM(___cpu_t_SIZEOF, sizeof(_cpu_t));

GEN_ABS_SYM_END
