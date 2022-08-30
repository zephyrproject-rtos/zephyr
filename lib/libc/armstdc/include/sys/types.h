/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This defines the required types for the ARMClang compiler when compiling with
 * _AEABI_PORTABILITY_LEVEL = 1
 *
 * The types defined are according to:
 * C Library ABI for the Arm® Architecture, 2021Q1
 *
 */
#ifndef ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TYPES_H_
#define ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TYPES_H_

#if !defined(__ssize_t_defined)
#define __ssize_t_defined

/* parasoft suppress item MISRAC2012-RULE_20_4-a item MISRAC2012-RULE_20_4-b
 * "Trick compiler to make sure the type of ssize_t won't be
 * unsigned long. View details in commit b889120"
 */
#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned
#endif

#if !defined(__off_t_defined)
#define __off_t_defined
typedef int off_t;
#endif

#if !defined(__time_t_defined)
#define __time_t_defined
typedef unsigned int time_t;
#endif

#if !defined(__clock_t_defined)
#define __clock_t_defined
typedef unsigned int clock_t;
#endif

#endif /* ZEPHYR_LIB_LIBC_ARMSTDC_INCLUDE_SYS_TYPES_H_ */
