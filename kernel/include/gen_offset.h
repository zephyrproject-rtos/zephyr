/*
 * Copyright (c) 2010, 2012, 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Macros to generate structure member offset definitions
 *
 * This header contains macros to allow a kernel implementation to generate
 * absolute symbols whose values represents the member offsets for various
 * kernel structures.  These absolute symbols are typically utilized by
 * assembly source files rather than hardcoding the values in some local header
 * file.
 *
 * WARNING: Absolute symbols can potentially be utilized by external tools --
 * for example, to locate a specific field within a data structure.
 * Consequently, changes made to such symbols may require modifications to the
 * associated tool(s). Typically, relocating a member of a structure merely
 * requires that a tool be rebuilt; however, moving a member to another
 * structure (or to a new sub-structure within an existing structure) may
 * require that the tool itself be modified. Likewise, deleting, renaming, or
 * changing the meaning of an absolute symbol may require modifications to a
 * tool.
 *
 * The macro "GEN_OFFSET_SYM(structure, member)" is used to generate a single
 * absolute symbol.  The absolute symbol will appear in the object module
 * generated from the source file that utilizes the GEN_OFFSET_SYM() macro.
 * Absolute symbols representing a structure member offset have the following
 * form:
 *
 *    __<structure>_<member>_OFFSET
 *
 * This header also defines the GEN_ABSOLUTE_SYM macro to simply define an
 * absolute symbol, irrespective of whether the value represents a structure
 * or offset.
 *
 * The following sample file illustrates the usage of the macros available
 * in this file:
 *
 *	<START of sample source file: offsets.c>
 *
 * #include <gen_offset.h>
 * /@ include struct definitions for which offsets symbols are to be
 * generated @/
 *
 * #include <kernel_structs.h>
 * GEN_ABS_SYM_BEGIN (_OffsetAbsSyms)	/@ the name parameter is arbitrary @/
 * /@ _kernel_t structure member offsets @/
 *
 * GEN_OFFSET_SYM (_kernel_t, nested);
 * GEN_OFFSET_SYM (_kernel_t, irq_stack);
 * GEN_OFFSET_SYM (_kernel_t, current);
 * GEN_OFFSET_SYM (_kernel_t, idle);
 *
 * GEN_ABSOLUTE_SYM (___kernel_t_SIZEOF, sizeof(_kernel_t));
 *
 * GEN_ABS_SYM_END
 * <END of sample source file: offsets.c>
 *
 * Compiling the sample offsets.c results in the following symbols in offsets.o:
 *
 * $ nm offsets.o
 * 00000000 A ___kernel_t_nested_OFFSET
 * 00000004 A ___kernel_t_irq_stack_OFFSET
 * 00000008 A ___kernel_t_current_OFFSET
 * 0000000c A ___kernel_t_idle_OFFSET
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_GEN_OFFSET_H_
#define ZEPHYR_KERNEL_INCLUDE_GEN_OFFSET_H_

#include <toolchain.h>
#include <stddef.h>

/* definition of the GEN_OFFSET_SYM() macros is toolchain independent  */

#define GEN_OFFSET_SYM(S, M) \
	GEN_ABSOLUTE_SYM(__##S##_##M##_##OFFSET, offsetof(S, M))

#endif /* ZEPHYR_KERNEL_INCLUDE_GEN_OFFSET_H_ */
