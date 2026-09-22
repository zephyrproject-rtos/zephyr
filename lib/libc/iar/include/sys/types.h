/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TYPES_H_
#define ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TYPES_H_

typedef unsigned int mode_t;
typedef signed long ssize_t;
typedef int off_t;
typedef __INT64_TYPE__ time_t;

#if !defined(_CLOCK_T_DECLARED) && !defined(__clock_t_defined)
typedef unsigned int clock_t;
#define _CLOCK_T_DECLARED
#define __clock_t_defined
#endif

#endif /* ZEPHYR_LIB_LIBC_IAR_INCLUDE_SYS_TYPES_H_ */
