/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-arch thread definition
 *
 * This file contains definitions for
 *
 *  struct _thread_arch
 *  struct _callee_saved
 *
 * necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/arch/arm64/mm.h>

struct _callee_saved {
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;
	uint64_t sp_el0;
	uint64_t sp_elx;
	uint64_t lr;
};

typedef struct _callee_saved _callee_saved_t;

struct z_arm64_fp_context {
	__int128 q0,  q1,  q2,  q3,  q4,  q5,  q6,  q7;
	__int128 q8,  q9,  q10, q11, q12, q13, q14, q15;
	__int128 q16, q17, q18, q19, q20, q21, q22, q23;
	__int128 q24, q25, q26, q27, q28, q29, q30, q31;
	uint32_t fpsr, fpcr;
};

struct _thread_arch {
#if defined(CONFIG_USERSPACE) || defined(CONFIG_ARM64_STACK_PROTECTION)
#if defined(CONFIG_ARM_MMU)
	struct arm_mmu_ptables *ptables;
#endif
#if defined(CONFIG_ARM_MPU)
	struct dynamic_region_info regions[ARM64_MPU_MAX_DYNAMIC_REGIONS];
	uint8_t region_num;
	atomic_t flushing;
#endif
#endif
#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	uint64_t stack_limit;
#endif
#ifdef CONFIG_FPU_SHARING
	struct z_arm64_fp_context saved_fp_context;
#endif
	uint8_t exception_depth;
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_H_ */
