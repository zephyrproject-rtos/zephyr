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

#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

#endif

#if !defined(__off_t_defined)
#define __off_t_defined

#if defined(__i386) || defined(__x86_64)
typedef long int off_t; /* "long" works for all of i386, X32 and true 64 bit */
#elif defined(__ARM_ARCH)
typedef int off_t;
#elif defined(__arc__)
typedef int off_t;
#elif defined(__NIOS2__)
typedef int off_t;
#elif defined(__riscv)
typedef int off_t;
#elif defined(__XTENSA__)
typedef int off_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

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
