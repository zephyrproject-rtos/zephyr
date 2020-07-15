/*
 * Copyright (c) 2018 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (SPARC)
 *
 * This file contains private kernel structures definitions and various
 * other definitions for the SPARC architecture.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

#ifndef ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

/**
 * @brief Sparc Standard Stack Frame
 *
 * From The SPARC Architecture Manual Version 8
 *
 * The following are always allocated at compile time in every
 * procedure’s stack frame:
 *
 * - 16 words, always starting at %sp, for saving the procedure’s in
 *   and local registers, should a register window overflow occur
 *
 * The following are allocated at compile time in the stack frames of
 * non-leaf procedures:
 *
 * - One word, for passing a “hidden” (implicit) parameter. This is
 *   used when the caller is expecting the callee to return a data
 *   aggregate by value; the hidden word contains the address of stack
 *   space allocated (if any) by the caller for that purpose See
 *   Section D.4.
 *
 * - Six words, into which the callee may store parameters that must
 *   be addressable
 *
 * The stack pointer %sp must always be doubleword-aligned. This
 * allows window overflow and underflow trap handlers to use the more
 * efficient STD and LDD instructions to store and reload register
 * windows.
 *
 */
struct _standard_stack_frame {
	uint32_t l0;
	uint32_t l1;
	uint32_t l2;
	uint32_t l3;
	uint32_t l4;
	uint32_t l5;
	uint32_t l6;
	uint32_t l7;

	uint32_t i0;
	uint32_t i1;
	uint32_t i2;
	uint32_t i3;
	uint32_t i4;
	uint32_t i5;
	uint32_t i6;
	uint32_t i7;

	uint32_t hidden;

	uint32_t arg1;
	uint32_t arg2;
	uint32_t arg3;
	uint32_t arg4;
	uint32_t arg5;
	uint32_t arg6;
};

typedef struct _standard_stack_frame _standard_stack_frame_t;

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_SPARC_INCLUDE_KERNEL_ARCH_DATA_H_ */
