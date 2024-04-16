/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPARC kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various SPARC kernel structures.
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_callee_saved_t, y);
GEN_OFFSET_SYM(_callee_saved_t, psr);

GEN_OFFSET_SYM(_callee_saved_t, l0_and_l1);
GEN_OFFSET_SYM(_callee_saved_t, l2);
GEN_OFFSET_SYM(_callee_saved_t, l4);
GEN_OFFSET_SYM(_callee_saved_t, l6);
GEN_OFFSET_SYM(_callee_saved_t, i0);
GEN_OFFSET_SYM(_callee_saved_t, i2);
GEN_OFFSET_SYM(_callee_saved_t, i4);
GEN_OFFSET_SYM(_callee_saved_t, i6);
GEN_OFFSET_SYM(_callee_saved_t, o6);

/* esf member offsets */
GEN_OFFSET_SYM(z_arch_esf_t, out);
GEN_OFFSET_SYM(z_arch_esf_t, global);
GEN_OFFSET_SYM(z_arch_esf_t, npc);
GEN_OFFSET_SYM(z_arch_esf_t, psr);
GEN_OFFSET_SYM(z_arch_esf_t, tbr);
GEN_ABSOLUTE_SYM(__z_arch_esf_t_SIZEOF, STACK_ROUND_UP(sizeof(z_arch_esf_t)));

GEN_ABS_SYM_END
