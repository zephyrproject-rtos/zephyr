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

GEN_OFFSET_STRUCT(arch_csf, y);
GEN_OFFSET_STRUCT(arch_csf, psr);

GEN_OFFSET_STRUCT(arch_csf, l0_and_l1);
GEN_OFFSET_STRUCT(arch_csf, l2);
GEN_OFFSET_STRUCT(arch_csf, l4);
GEN_OFFSET_STRUCT(arch_csf, l6);
GEN_OFFSET_STRUCT(arch_csf, i0);
GEN_OFFSET_STRUCT(arch_csf, i2);
GEN_OFFSET_STRUCT(arch_csf, i4);
GEN_OFFSET_STRUCT(arch_csf, i6);
GEN_OFFSET_STRUCT(arch_csf, o6);

/* esf member offsets */
GEN_OFFSET_STRUCT(arch_esf, out);
GEN_OFFSET_STRUCT(arch_esf, global);
GEN_OFFSET_STRUCT(arch_esf, npc);
GEN_OFFSET_STRUCT(arch_esf, psr);
GEN_OFFSET_STRUCT(arch_esf, tbr);
GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
