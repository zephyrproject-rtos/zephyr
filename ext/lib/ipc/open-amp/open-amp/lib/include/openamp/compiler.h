#ifndef _COMPILER_H_
#define _COMPILER_H_

/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**************************************************************************
 * FILE NAME
 *
 *       compiler.h
 *
 * DESCRIPTION
 *
 *       This file defines compiler-specific macros.
 *
 ***************************************************************************/
#if defined __cplusplus
extern "C" {
#endif

/* IAR ARM build tools */
#if defined(__ICCARM__)

#ifndef OPENAMP_PACKED_BEGIN
#define OPENAMP_PACKED_BEGIN __packed
#endif

#ifndef OPENAMP_PACKED_END
#define OPENAMP_PACKED_END
#endif

/* GNUC */
#elif defined(__GNUC__)

#ifndef OPENAMP_PACKED_BEGIN
#define OPENAMP_PACKED_BEGIN
#endif

#ifndef OPENAMP_PACKED_END
#define OPENAMP_PACKED_END __attribute__((__packed__))
#endif

/* ARM GCC */
#elif defined(__CC_ARM)

#ifndef OPENAMP_PACKED_BEGIN
#define OPENAMP_PACKED_BEGIN _Pragma("pack(1U)")
#endif

#ifndef OPENAMP_PACKED_END
#define OPENAMP_PACKED_END _Pragma("pack()")
#endif

#else
/* There is no default definition here to avoid wrong structures packing in case of not supported compiler */
#error Please implement the structure packing macros for your compiler here!
#endif

#if defined __cplusplus
}
#endif

#endif /* _COMPILER_H_ */
