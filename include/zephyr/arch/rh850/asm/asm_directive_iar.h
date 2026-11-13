/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_IAR_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_IAR_H_

#ifdef _ASMLANGUAGE

#ifndef __IAR_SYSTEMS_ASM__
#error "This header is only available to IAR"
#endif /* __IAR_SYSTEMS_ASM__ */

#define RH850_ASM_SECTION_TYPE_CODE        CODE
#define RH850_ASM_SECTION_TYPE_CONST       CONST
#define RH850_ASM_SECTION_TYPE_CODE_ROOT   CODE:ROOT

#define RH850_ASM_DIRECTIVE_MACRO(...)   __VA_ARGS__ MACRO
#define RH850_ASM_DIRECTIVE_MACRO_END    ENDM
#define RH850_ASM_DIRECTIVE_EXTERN(...)  EXTERN __VA_ARGS__
#define RH850_ASM_DIRECTIVE_WEAK(...)    EXTWEAK __VA_ARGS__
#define RH850_ASM_DIRECTIVE_P2ALIGN(...) ALIGN __VA_ARGS__
#define RH850_ASM_DIRECTIVE_SECTION(type, p2align, ...)                                            \
	SECTION __VA_ARGS__:RH850_ASM_SECTION_TYPE_##type(p2align)
#define RH850_ASM_DIRECTIVE_GLOBAL(...)   PUBLIC __VA_ARGS__
#define RH850_ASM_DIRECTIVE_EQU(val, ...) __VA_ARGS__ EQU val
#define RH850_ASM_DIRECTIVE_SET(val, ...) __VA_ARGS__ SET val
#define RH850_ASM_DIRECTIVE_END           END

#define RH850_HI(x) hi1(x)
#define RH850_LO(x) lw1(x)

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_IAR_H_ */
