/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_EXC_HANDLE_H_
#define ZEPHYR_INCLUDE_EXC_HANDLE_H_

/*
 * This is used by some architectures to define code ranges which may
 * perform operations that could generate a CPU exception that should not
 * be fatal. Instead, the exception should return but set the program
 * counter to a 'fixup' memory address which will gracefully error out.
 *
 * For example, in the case where user mode passes in a C string via
 * system call, the length of that string needs to be measured. A specially
 * written assembly language version of strlen (z_arch_user_string_len)
 * defines start and end symbols where the memory in the string is examined;
 * if this generates a fault, jumping to the fixup symbol within the same
 * function will return an error result to the caller.
 *
 * To ensure precise control of the state of registers and the stack pointer,
 * these functions need to be written in assembly.
 *
 * The arch-specific fault handling code will define an array of these
 * z_exc_handle structures and return from the exception with the PC updated
 * to the fixup address if a match is found.
 */

struct z_exc_handle {
	void *start;
	void *end;
	void *fixup;
};

#define Z_EXC_HANDLE(name) \
	{ name ## _fault_start, name ## _fault_end, name ## _fixup }

#define Z_EXC_DECLARE(name)          \
	void name ## _fault_start(void); \
	void name ## _fault_end(void);   \
	void name ## _fixup(void)

#endif /* ZEPHYR_INCLUDE_EXC_HANDLE_H_ */
