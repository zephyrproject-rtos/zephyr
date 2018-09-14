/* asm.h - x86 tool dependent headers */

/*
 * Copyright (c) 2007-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ASM_H_
#define ZEPHYR_INCLUDE_ARCH_X86_ASM_H_


#include <toolchain.h>
#include <linker/sections.h>

/* offsets from stack pointer to function arguments */

#define SP_ARG0         0
#define SP_ARG1         4
#define SP_ARG2         8
#define SP_ARG3         12
#define SP_ARG4         16
#define SP_ARG5         20
#define SP_ARG6         24
#define SP_ARG7         28
#define SP_ARG8         32

#if defined(_ASMLANGUAGE)

#if defined(CONFIG_RETPOLINE)
/*
 * For a description of how retpolines are constructed for both indirect
 * jumps and indirect calls, please refer to this documentation:
 * https://support.google.com/faqs/answer/7625886
 *
 * Since these macros are used in a few places in arch/x86/core assembly
 * routines, with different reg parameters, it's not possible to use
 * the "out of line" construction technique to share a trampoline.
 */

#define INDIRECT_JMP_IMPL(reg, id) \
		call .set_up_target ## id; \
	.speculative_trap ## id: \
		pause; \
		jmp .speculative_trap ## id; \
	.set_up_target ## id: \
		mov reg, (%esp); \
		ret

#define INDIRECT_CALL_IMPL(reg, id) \
		call .set_up_return ## id; \
	.inner_indirect_branch ## id: \
		call .set_up_target ## id; \
	.speculative_trap ## id: \
		pause; \
		jmp .speculative_trap ## id; \
	.set_up_target ## id: \
		mov reg, (%esp); \
		ret; \
	.set_up_return ## id: \
		call .inner_indirect_branch ## id


#define INDIRECT_CALL_IMPL1(reg, id)	INDIRECT_CALL_IMPL(reg, id)
#define INDIRECT_JMP_IMPL1(reg, id)	INDIRECT_JMP_IMPL(reg, id)

#define INDIRECT_CALL(reg)	INDIRECT_CALL_IMPL1(reg, __COUNTER__)
#define INDIRECT_JMP(reg)	INDIRECT_JMP_IMPL1(reg, __COUNTER__)

#else

#define INDIRECT_CALL(reg)	call *reg
#define INDIRECT_JMP(reg)	jmp *reg

#endif /* CONFIG_RETPOLINE */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ASM_H_ */
