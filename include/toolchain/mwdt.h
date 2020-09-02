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

#include <toolchain/common.h>

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

#include <toolchain/gcc.h>

/* Metaware toolchain has _Static_assert. However it not able to calculate
 * conditional expression in build time for some realy complex cases. ARC GNU
 * toolchain works fine in this cases, so it looks like MWDT bug. So, disable
 * BUILD_ASSERT macro until we fix that issue in MWDT toolchain.
 */
#undef BUILD_ASSERT
#define BUILD_ASSERT(EXPR, MSG...)

#define __builtin_arc_nop()	_nop()

#endif /* _ASMLANGUAGE */

#endif /* !_LINKER */
#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_MWDT_H_ */
