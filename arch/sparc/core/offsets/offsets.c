/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPARC kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various SPARC kernel structures.
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

#include <kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

GEN_OFFSET_SYM(_thread_t, switch_handle);

GEN_OFFSET_SYM(_standard_stack_frame_t, l0);
GEN_OFFSET_SYM(_standard_stack_frame_t, l1);
GEN_OFFSET_SYM(_standard_stack_frame_t, l2);
GEN_OFFSET_SYM(_standard_stack_frame_t, l3);
GEN_OFFSET_SYM(_standard_stack_frame_t, l4);
GEN_OFFSET_SYM(_standard_stack_frame_t, l5);
GEN_OFFSET_SYM(_standard_stack_frame_t, l6);
GEN_OFFSET_SYM(_standard_stack_frame_t, l7);
GEN_OFFSET_SYM(_standard_stack_frame_t, i0);
GEN_OFFSET_SYM(_standard_stack_frame_t, i1);
GEN_OFFSET_SYM(_standard_stack_frame_t, i2);
GEN_OFFSET_SYM(_standard_stack_frame_t, i3);
GEN_OFFSET_SYM(_standard_stack_frame_t, i4);
GEN_OFFSET_SYM(_standard_stack_frame_t, i5);
GEN_OFFSET_SYM(_standard_stack_frame_t, i6);
GEN_OFFSET_SYM(_standard_stack_frame_t, i7);
GEN_OFFSET_SYM(_standard_stack_frame_t, hidden);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg1);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg2);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg3);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg4);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg5);
GEN_OFFSET_SYM(_standard_stack_frame_t, arg6);

GEN_ABSOLUTE_SYM(__STD_STACK_FRAME_SIZEOF,
		 STACK_ROUND_UP(sizeof(_standard_stack_frame_t)));

GEN_ABS_SYM_END
