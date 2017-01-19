/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_sys_types_h__
#define __INC_sys_types_h__

#if !defined(__ssize_t_defined)
#define __ssize_t_defined

#ifdef __i386
typedef long int ssize_t;
#elif defined(__ARM_ARCH)
typedef int ssize_t;
#elif defined(__arc__)
typedef int ssize_t;
#elif defined(__NIOS2__)
typedef int ssize_t;
#elif defined(__riscv__)
typedef int ssize_t;
#else
#error "The minimal libc library does not recognize the architecture!\n"
#endif

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
