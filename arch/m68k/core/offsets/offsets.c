/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>
#include <kernel_offsets.h>
#include <zephyr/arch/m68k/exception.h>

BUILD_ASSERT(offsetof(struct arch_esf, d0) == 0U);
BUILD_ASSERT(offsetof(struct arch_esf, d1) == 4U);
BUILD_ASSERT(offsetof(struct arch_esf, a0) == 8U);
BUILD_ASSERT(offsetof(struct arch_esf, a1) == 12U);
BUILD_ASSERT(offsetof(struct arch_esf, vector) == 16U);
BUILD_ASSERT(offsetof(struct arch_esf, sr) == 20U);
BUILD_ASSERT(offsetof(struct arch_esf, pc) == 22U);

#if defined(CONFIG_M68K_EXCEPTION_FRAME_HAS_FORMAT_WORD)
BUILD_ASSERT(offsetof(struct arch_esf, format_vector) == 26U);
BUILD_ASSERT(sizeof(struct arch_esf) == 28U);
#else
BUILD_ASSERT(sizeof(struct arch_esf) == 26U);
#endif

GEN_OFFSET_SYM(_callee_saved_t, sp);

GEN_OFFSET_STRUCT(arch_esf, d0);
GEN_OFFSET_STRUCT(arch_esf, d1);
GEN_OFFSET_STRUCT(arch_esf, a0);
GEN_OFFSET_STRUCT(arch_esf, a1);
GEN_OFFSET_STRUCT(arch_esf, vector);
GEN_OFFSET_STRUCT(arch_esf, sr);
GEN_OFFSET_STRUCT(arch_esf, pc);
#if defined(CONFIG_M68K_EXCEPTION_FRAME_HAS_FORMAT_WORD)
GEN_OFFSET_STRUCT(arch_esf, format_vector);
#endif
GEN_ABSOLUTE_SYM(__struct_arch_esf_SIZEOF, sizeof(struct arch_esf));

GEN_ABS_SYM_END
