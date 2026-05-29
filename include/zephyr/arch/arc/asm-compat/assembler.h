/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (C) 2021 Synopsys, Inc. (www.synopsys.com)
 *
 * Author: Vineet Gupta <vgupta@synopsys.com>
 *
 * Top level include file providing ISA pseudo-mnemonics for use in assembler
 * and inline assembly.
 *
 *  - Helps code reuse across ARC64/ARC32/ARCv2
 *    e.g. "LDR" maps to 'LD' on 32-bit ISA, 'LDL' on 64-bit ARCv2/ARC64
 *
 *  - Provides emulation with multiple instructions if the case be
 *    e.g. "DBNZ" implemented using 'SUB' and 'BRNE'
 *
 *  - Looks more complex than it really is: mainly because Kconfig defines
 *    are not "honored" in inline assembly. So each variant is unconditional
 *    code in a standalone file with Kconfig based #ifdef'ry here. During the
 *    build process, the "C" preprocessor runs through this file, leaving
 *    just the final variant include in code fed to compiler/assembler.
 */

#ifndef __ASM_ARC_ASM_H
#define __ASM_ARC_ASM_H 1

#ifdef _ASMLANGUAGE

#if defined(CONFIG_ISA_ARCV3) && defined(CONFIG_64BIT)
#define ARC_PTR		.xword
#define ARC_REGSZ	8
#define ARC_REGSHIFT	3

#if defined(__CCAC__)
#include "asm-macro-64-bit-mwdt.h"
#else
#include "asm-macro-64-bit-gnu.h"
#endif /* defined(__CCAC__) */

#elif defined(CONFIG_ISA_ARCV3) && !defined(CONFIG_64BIT)
#define ARC_PTR		.word
#define ARC_REGSZ	4
#define ARC_REGSHIFT	2

#if defined(__CCAC__)
#include "asm-macro-32-bit-mwdt.h"
#else
#include "asm-macro-32-bit-gnu.h"
#endif /* defined(__CCAC__) */

#else
#define ARC_PTR		.word
#define ARC_REGSZ	4
#define ARC_REGSHIFT	2

#if defined(__CCAC__)
#include "asm-macro-32-bit-mwdt.h"
#else
#include "asm-macro-32-bit-gnu.h"
#endif /* defined(__CCAC__) */

#endif

#else	/* !_ASMLANGUAGE */

#error "asm-compat macroses used not in assembler code!"

#endif	/* _ASMLANGUAGE */

#endif
