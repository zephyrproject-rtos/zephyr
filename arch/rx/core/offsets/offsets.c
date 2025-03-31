/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RX Kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various structures.
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
#ifndef _RX_OFFSETS_INC_
#define _RX_OFFSETS_INC_

#include <gen_offset.h>
#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <kernel_offsets.h>

GEN_ABSOLUTE_SYM(__callee_saved_t_SIZEOF, sizeof(_callee_saved_t));
GEN_ABSOLUTE_SYM(__thread_arch_t_SIZEOF, sizeof(_thread_arch_t));

GEN_ABS_SYM_END

#endif /* _RX_OFFSETS_INC_ */
