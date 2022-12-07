/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 ARM Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_

#include <stdint.h>
#include <sys/_types.h>

typedef unsigned int mode_t;

#if !defined(__ssize_t_defined)
#define __ssize_t_defined

/* Static code analysis tool can raise a violation
 * in the line below where name of macro 'unsigned' is the same
 * as keyword. It is made on purpose, deliberated deviation.
 *
 * We trick compiler to make sure the type of ssize_t won't be unsigned long.
 * As otherwise the type of ssize_t will be unsigned long
 * which is not correct. More details view in commit b889120
 */
#define unsigned signed /* parasoft-suppress MISRAC2012-RULE_20_4-a MISRAC2012-RULE_20_4-b */
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

#endif

#if !defined(__off_t_defined)
#define __off_t_defined
/* off_t is defined such that it matches the size of a pointer */
typedef __INTPTR_TYPE__ off_t;
#endif

#if !defined(__time_t_defined)
#define __time_t_defined
typedef _TIME_T_ time_t;
#endif

#if !defined(__suseconds_t_defined)
#define __suseconds_t_defined
typedef _SUSECONDS_T_ suseconds_t;
#endif

#if !defined(__mem_word_t_defined)
#define __mem_word_t_defined

/*
 * The mem_word_t should match the optimal memory access word width
 * on the target platform. Here we defaults it to uintptr_t.
 */

typedef uintptr_t mem_word_t;

#define Z_MEM_WORD_T_WIDTH __INTPTR_WIDTH__

#endif

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_ */
