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
	uint32_t pc;
	uint32_t npc;
	uint32_t psr;
	uint32_t y;

	uint32_t o0;
	uint32_t o1;
	uint32_t o2;
	uint32_t o3;
	uint32_t o4;
	uint32_t o5;
	/* %sp is stored on switch_handle */
	uint32_t o7;

	uint32_t g1;
	uint32_t g2;
	uint32_t g3;
	uint32_t g4;
	uint32_t g5;
	uint32_t g6;
	uint32_t g7;
};

typedef struct __esf z_arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_EXP_H_ */
