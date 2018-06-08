/*
 * Copyright (c) 2019 ispace, inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPARC public exception handling
 *
 * RISCV-specific kernel exception handling interface.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_EXP_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_EXP_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exception Stack Frame
 *
 * ESF, Exception Stack Frame, is a stack frame used to save CPU
 * context when exception occur.  We also use this stack frame with
 * context switches in both cases of cooperative z_swap() and
 * preemption after a trap.
 *
 * The Zephyr build system generates offsets to each position from
 * this structure.  See offsets/offsets.c.
 *
 */
struct __esf {
	u32_t pc;
	u32_t npc;
	u32_t psr;
	u32_t y;

	u32_t o0;
	u32_t o1;
	u32_t o2;
	u32_t o3;
	u32_t o4;
	u32_t o5;
	/* %sp is stored on switch_handle */
	u32_t o7;

	u32_t g1;
	u32_t g2;
	u32_t g3;
	u32_t g4;
	u32_t g5;
	u32_t g6;
	u32_t g7;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_EXP_H_ */
