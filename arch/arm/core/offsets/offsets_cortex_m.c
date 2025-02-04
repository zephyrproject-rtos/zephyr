/*
 * Copyright (c) 2025 Arm ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM kernel structure member offset definition file
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

#ifndef _ARM_OFFSETS_CORTEX_M_INC_
#define _ARM_OFFSETS_CORTEX_M_INC_

#include "zephyr/toolchain.h"
#include <gen_offset.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

#ifdef CONFIG_USE_SWITCH
GEN_OFFSET_SYM(_cpu_t, current);
GEN_ABSOLUTE_SYM(___cpu_t_SIZEOF, sizeof(_cpu_t));
GEN_OFFSET_SYM(_callee_saved_t, lr);
GEN_OFFSET_SYM(_basic_sf_t, pc);
GEN_OFFSET_SYM(_basic_sf_t, r0);
GEN_OFFSET_SYM(_basic_sf_t, r1);
#endif

#endif /* _ARM_OFFSETS_INC_ */
