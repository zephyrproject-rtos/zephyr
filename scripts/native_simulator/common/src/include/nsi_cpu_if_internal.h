/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_CPU_IF_INTERNAL_H
#define NSI_COMMON_SRC_INCL_NSI_CPU_IF_INTERNAL_H

#include "nsi_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FUNCT(i, pre, post) \
	pre##i##post

#define FUNCT_LIST(pre, post, sep) \
	FUNCT(0, pre, post) NSI_DEBRACKET sep \
	FUNCT(1, pre, post) NSI_DEBRACKET sep \
	FUNCT(2, pre, post) NSI_DEBRACKET sep \
	FUNCT(3, pre, post) NSI_DEBRACKET sep \
	FUNCT(4, pre, post) NSI_DEBRACKET sep \
	FUNCT(5, pre, post) NSI_DEBRACKET sep \
	FUNCT(6, pre, post) NSI_DEBRACKET sep \
	FUNCT(7, pre, post) NSI_DEBRACKET sep \
	FUNCT(8, pre, post) NSI_DEBRACKET sep \
	FUNCT(9, pre, post) NSI_DEBRACKET sep \
	FUNCT(10, pre, post) NSI_DEBRACKET sep \
	FUNCT(11, pre, post) NSI_DEBRACKET sep \
	FUNCT(12, pre, post) NSI_DEBRACKET sep \
	FUNCT(13, pre, post) NSI_DEBRACKET sep \
	FUNCT(14, pre, post) NSI_DEBRACKET sep \
	FUNCT(15, pre, post) NSI_DEBRACKET sep \

#define F_TRAMP_TABLE(pre, post) FUNCT_LIST(pre, post, (,))

#define F_TRAMP_LIST(pre, post) FUNCT_LIST(pre, post, (;))

#define F_TRAMP_BODY_LIST(pre, post) FUNCT_LIST(pre, post, ())

#define TRAMPOLINES(pre, post)             \
	void pre ## n ## post(int n)       \
	{                                  \
		void(*fptrs[])(void) = {   \
			F_TRAMP_TABLE(pre, post) \
		};                         \
		fptrs[n]();                \
	}

#define TRAMPOLINES_i_vp(pre, post)           \
	int pre ## n ## post(int n, void *p) \
	{                                    \
		int(*fptrs[])(void *p) = {   \
			F_TRAMP_TABLE(pre, post) \
		};                           \
		return fptrs[n](p);          \
	}

#define TRAMPOLINES_i_(pre, post)             \
	int pre ## n ## post(int n)          \
	{                                    \
		int(*fptrs[])(void) = {      \
			F_TRAMP_TABLE(pre, post) \
		};                           \
		return fptrs[n]();           \
	}

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_CPU_IF_INTERNAL_H */
