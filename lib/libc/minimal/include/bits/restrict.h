/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief _Restrict definition
 *
 * The macro "_Restrict" is intended to be private to the minimal libc library.
 * It evaluates to the "restrict" keyword when a C99 compiler is used, and
 * to "__restrict__" when a C++ compiler is used.
 */

#if !defined(_Restrict_defined)
#define _Restrict_defined

#ifdef __cplusplus
	#define _Restrict __restrict__
#else
	#define _Restrict restrict
#endif

#endif
