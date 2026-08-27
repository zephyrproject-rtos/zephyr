/*
 * Copyright (c) 2020 Synopsys.
 * Author: Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_TOOL_COMPAT_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_TOOL_COMPAT_H_

#ifdef _ASMLANGUAGE
/*
 * GNU toolchain and MWDT (Metware) toolchain have different style for accessing
 * arguments in assembly macro. Here is the preprocessor macro to handle the
 * difference.
 * __CCAC__ is a pre-defined macro of metaware compiler.
 */
#if defined(__CCAC__)
#define MACRO_ARG(x) x
#else
#define MACRO_ARG(x) \x
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARC_TOOL_COMPAT_H_ */
