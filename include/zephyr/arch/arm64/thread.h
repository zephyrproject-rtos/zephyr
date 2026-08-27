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
#ifdef CONFIG_ARM_PAC_PER_THREAD
#include <zephyr/arch/arm64/pac.h>
#endif

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
	uint32_t fpsr, fpcr;
	union {
		struct {
			__int128 v_regs[32];
		} neon;
#ifdef CONFIG_ARM64_SVE
		struct {
			uint8_t z_regs[32 * CONFIG_ARM64_SVE_VL_MAX] __aligned(8);
			uint8_t p_regs[16 * (CONFIG_ARM64_SVE_VL_MAX / 8)];
			uint8_t ffr[CONFIG_ARM64_SVE_VL_MAX / 8];
			enum { SIMD_NONE = 0, SIMD_NEON, SIMD_SVE } simd_mode;
		} sve;
#endif
	};
};

typedef struct z_arm64_fp_context z_arm64_fp_context;

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
	uint8_t exception_depth;
	/* Keep large structures at the end to avoid offset issues */
#ifdef CONFIG_FPU_SHARING
	struct z_arm64_fp_context saved_fp_context;
#endif
#ifdef CONFIG_ARM_PAC_PER_THREAD
	struct pac_keys pac_keys;
#endif
};

typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_THREAD_H_ */
