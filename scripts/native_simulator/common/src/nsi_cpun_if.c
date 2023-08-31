/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsi_cpu_if.h"

/*
 * These trampolines forward a call from the runner into the corresponding embedded CPU hook
 * for ex., nsif_cpun_boot(4) -> nsif_cpu4_boot()
 */

#define FUNCT(i, pre, post) \
	pre##i##post

#define F_TABLE(pre, post)    \
	FUNCT(0, pre, post), \
	FUNCT(1, pre, post), \
	FUNCT(2, pre, post), \
	FUNCT(3, pre, post), \
	FUNCT(4, pre, post), \
	FUNCT(5, pre, post), \
	FUNCT(6, pre, post), \
	FUNCT(7, pre, post), \
	FUNCT(8, pre, post), \
	FUNCT(9, pre, post), \
	FUNCT(10, pre, post), \
	FUNCT(11, pre, post), \
	FUNCT(12, pre, post), \
	FUNCT(13, pre, post), \
	FUNCT(14, pre, post), \
	FUNCT(15, pre, post)

#define TRAMPOLINES(pre, post)             \
	void pre ## n ## post(int n)       \
	{                                  \
		void(*fptrs[])(void) = {   \
			F_TABLE(pre, post) \
		};                         \
		fptrs[n]();                \
	}

#define TRAMPOLINES_i(pre, post)             \
	int pre ## n ## post(int n)          \
	{                                    \
		int(*fptrs[])(void) = {      \
			F_TABLE(pre, post)   \
		};                           \
		return fptrs[n]();           \
	}

TRAMPOLINES(nsif_cpu, _pre_cmdline_hooks)
TRAMPOLINES(nsif_cpu, _pre_hw_init_hooks)
TRAMPOLINES(nsif_cpu, _boot)
TRAMPOLINES_i(nsif_cpu, _cleanup)
TRAMPOLINES(nsif_cpu, _irq_raised)
TRAMPOLINES(nsif_cpu, _irq_raised_from_sw)
