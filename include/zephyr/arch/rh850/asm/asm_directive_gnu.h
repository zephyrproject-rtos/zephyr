/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_GNU_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_GNU_H_

#ifdef _ASMLANGUAGE

#ifndef __ASSEMBLER__
#error "This header is only available to GNU"
#endif /* __ASSEMBLER__ */

#define RH850_ASM_SECTION_FLAGS_CODE      "ax"
#define RH850_ASM_SECTION_FLAGS_CONST     "a"
#define RH850_ASM_SECTION_FLAGS_CODE_ROOT "ax"

#define RH850_ASM_DIRECTIVE_MACRO(...)   .macro __VA_ARGS__
#define RH850_ASM_DIRECTIVE_MACRO_END    .endm
#define RH850_ASM_DIRECTIVE_EXTERN(...)  .extern __VA_ARGS__
#define RH850_ASM_DIRECTIVE_WEAK(...)    .weak __VA_ARGS__
#define RH850_ASM_DIRECTIVE_P2ALIGN(...) .p2align __VA_ARGS__
#define RH850_ASM_DIRECTIVE_SECTION(type, align, ...)                                              \
	.section __VA_ARGS__, RH850_ASM_SECTION_FLAGS_##type;                                      \
	.p2align align
#define RH850_ASM_DIRECTIVE_GLOBAL(...)   .global __VA_ARGS__
#define RH850_ASM_DIRECTIVE_EQU(val, ...) .equ __VA_ARGS__, val
#define RH850_ASM_DIRECTIVE_SET(val, ...) .set __VA_ARGS__, val
#define RH850_ASM_DIRECTIVE_END           .end

#define RH850_HI(x) hi(x)
#define RH850_LO(x) lo(x)

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_GNU_H_ */
