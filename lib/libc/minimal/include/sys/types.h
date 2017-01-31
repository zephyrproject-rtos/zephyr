/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 ARM Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_sys_types_h__
#define __INC_sys_types_h__

#if !defined(__ssize_t_defined)
#define __ssize_t_defined

#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned

#endif

#if !defined(__off_t_defined)
#define __off_t_defined

#ifdef __i386
typedef long int off_t;
#elif defined(__ARM_ARCH)
typedef int off_t;
#elif defined(__arc__)
typedef int off_t;
#elif defined(__NIOS2__)
typedef int off_t;
#elif defined(__riscv__)
typedef int off_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

#endif

#endif /* __INC_sys_types_h__ */
