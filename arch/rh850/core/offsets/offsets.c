/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>

#include <kernel_offsets.h>

BUILD_ASSERT(offsetof(struct arch_esf, r31) == offsetof(struct arch_esf, r4) + 27 * 4);

GEN_OFFSET_SYM(_cpu_arch_t, esf);

GEN_OFFSET_STRUCT(arch_esf, r1);
GEN_OFFSET_STRUCT(arch_esf, r2);
GEN_OFFSET_STRUCT(arch_esf, r4);
GEN_OFFSET_STRUCT(arch_esf, eipc);
GEN_OFFSET_STRUCT(arch_esf, eipsw);
GEN_OFFSET_STRUCT(arch_esf, fpsr);
GEN_OFFSET_STRUCT(arch_esf, fpepc);
GEN_OFFSET_STRUCT(arch_esf, ctpc);
GEN_OFFSET_STRUCT(arch_esf, ctpsw);
GEN_ABSOLUTE_SYM(___esf_t_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
