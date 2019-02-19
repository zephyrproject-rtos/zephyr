/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 ARM Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_

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

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SYS_TYPES_H_ */
