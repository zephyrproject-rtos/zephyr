/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief _MLIBC_RESTRICT definition
 *
 * The macro "_MLIBC_RESTRICT" is intended to be private to the minimal libc
 * library.  It evaluates to the "restrict" keyword when a C99 compiler is
 * used, and to "__restrict__" when a C++ compiler is used.
 */

#if !defined(_MLIBC_RESTRICT_defined)
#define _MLIBC_RESTRICT_defined

#ifdef __cplusplus
	#define _MLIBC_RESTRICT __restrict__
#else
	#define _MLIBC_RESTRICT restrict
#endif

#endif
