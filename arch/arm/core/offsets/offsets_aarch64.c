/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various ARM kernel structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final kernel ELF image (due to the linker's reference to the _OffsetAbsSyms
 * symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

#ifndef _ARM_OFFSETS_INC_
#define _ARM_OFFSETS_INC_

#include <kernel.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_NAMED_OFFSET_SYM(_callee_saved_t, x19, x19_x20);
GEN_NAMED_OFFSET_SYM(_callee_saved_t, x21, x21_x22);
GEN_NAMED_OFFSET_SYM(_callee_saved_t, x23, x23_x24);
GEN_NAMED_OFFSET_SYM(_callee_saved_t, x25, x25_x26);
GEN_NAMED_OFFSET_SYM(_callee_saved_t, x27, x27_x28);
GEN_NAMED_OFFSET_SYM(_callee_saved_t, x29, x29_sp);

GEN_ABSOLUTE_SYM(___callee_saved_t_SIZEOF, sizeof(struct _callee_saved));

#endif /* _ARM_OFFSETS_INC_ */
