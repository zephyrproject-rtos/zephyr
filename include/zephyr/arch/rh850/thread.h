/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RH850 specific thread definitions
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

/** @cond INTERNAL_HIDDEN */

struct _callee_saved {
	uint32_t unused;
};

typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
#ifdef CONFIG_FXU
	/* set on the first FXU instruction of the thread */
	uint32_t fxu_user;
	/* FXSR, FXXC */
	uint32_t fxu_sr[2];
	/* wr0-wr31, 16-byte aligned at run time */
	uint8_t fxu_wr[32 * 16 + 15];
#endif
};

typedef struct _thread_arch _thread_arch_t;

/** @endcond */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_THREAD_H_ */
