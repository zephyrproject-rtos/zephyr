/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_H_

#ifdef _ASMLANGUAGE

#if defined(__IAR_SYSTEMS_ASM__)
#include "asm_directive_iar.h"
#elif defined(__ASSEMBLER__)
#include "asm_directive_gnu.h"
#else
#error "Unsupported toolchain"
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_ASM_DIRECTIVE_H_ */
