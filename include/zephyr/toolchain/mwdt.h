/*
 * Copyright (c) 2020 Synopsys.
 * Author: Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_

#ifndef _LINKER
#if defined(_ASMLANGUAGE)

#include <zephyr/toolchain/common.h>

#define FUNC_CODE()
#define FUNC_INSTR(a)

.macro section_var_mwdt, section, symbol
	.section .\&section\&.\&symbol, "aw"
	symbol :
.endm

.macro section_func_mwdt, section, symbol
	.section .\&section\&.\&symbol, "ax"
	FUNC_CODE()
	PERFOPT_ALIGN
	symbol :
	FUNC_INSTR(symbol)
.endm

.macro section_subsec_func_mwdt, section, subsection, symbol
	.section .\&section\&.\&subsection, "ax"
	PERFOPT_ALIGN
	symbol :
.endm

#define SECTION_VAR(sect, sym) section_var_mwdt sect, sym
#define SECTION_FUNC(sect, sym) section_func_mwdt sect, sym
#define SECTION_SUBSEC_FUNC(sect, subsec, sym) \
	section_subsec_func_mwdt sect, subsec, sym

.macro glbl_text_mwdt, symbol
	.globl symbol
	.type symbol, @function
.endm

.macro glbl_data_mwdt, symbol
	.globl symbol
	.type symbol, @object
.endm

.macro weak_data_mwdt, symbol
	.weak symbol
	.type symbol, @object
.endm

#define GTEXT(sym) glbl_text_mwdt sym
#define GDATA(sym) glbl_data_mwdt sym
#define WDATA(sym) weak_data_mwdt sym

#else /* defined(_ASMLANGUAGE) */

/* MWDT toolchain misses ssize_t definition which is used by Zephyr */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#ifdef CONFIG_64BIT
	typedef long ssize_t;
#else
	typedef int ssize_t;
#endif
#endif /* _SSIZE_T_DEFINED */

#ifdef CONFIG_NEWLIB_LIBC
  #error "ARC MWDT doesn't support building with CONFIG_NEWLIB_LIBC as it doesn't have newlib"
#endif /* CONFIG_NEWLIB_LIBC */

#ifdef CONFIG_NATIVE_APPLICATION
  #error "ARC MWDT doesn't support building Zephyr as an native application"
#endif /* CONFIG_NATIVE_APPLICATION */


#define __no_optimization __attribute__((optnone))
#define __fallthrough     __attribute__((fallthrough))

#define TOOLCHAIN_HAS_C_GENERIC                 1
#define TOOLCHAIN_HAS_C_AUTO_TYPE               1

#include <zephyr/toolchain/gcc.h>

#undef BUILD_ASSERT
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define BUILD_ASSERT(EXPR, MSG...) static_assert(EXPR, "" MSG)
#elif defined(__cplusplus)
/* For cpp98 */
#define BUILD_ASSERT(EXPR, MSG...)
#else
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert(EXPR, "" MSG)
#endif

#define __builtin_arc_nop()	_nop()

#endif /* _ASMLANGUAGE */

#endif /* !_LINKER */
#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_ */
