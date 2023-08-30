/* Copyright 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_WIN0_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_WIN0_H_

#include <xtensa/config/core-isa.h>

struct xtensa_win0_ctx {
	uint32_t a0;
	uint32_t a1;
	uint32_t a2;
	uint32_t a3;
	uint32_t a4;
	uint32_t a5;
	uint32_t a6;
	uint32_t a7;
	uint32_t a8;
	uint32_t a9;
	uint32_t a10;
	uint32_t a11;
	uint32_t a12;
	uint32_t a13;
	uint32_t a14;
	uint32_t a15;
	uint32_t ps;
	uint32_t pc;
	uint32_t sar;
#if XCHAL_HAVE_LOOPS
	uint32_t lcount;
	uint32_t lend;
	uint32_t lbeg;
#endif
#if XCHAL_HAVE_S32C1I
	uint32_t scompare1;
#endif
#if XCHAL_HAVE_THREADPTR
	/* Note: not actually mutable context.  It's always equal to
	 * the thread::tls field, maybe just use that?
	 */
	uint32_t threadptr;
#endif
#ifdef CONFIG_FPU_SHARING
	uint32_t fcr;
	uint32_t fsr;
	uint32_t fregs[16];
#endif
/* #ifdef CONFIG_USERSPACE */
	/* Always enabled currently to support a syscall mocking layer */
	uint32_t user_a0;
	uint32_t user_a1;
	uint32_t user_pc;
	uint32_t user_ps;
/* #endif */
};

/* A type name is needed for GEN_OFFSET_SYM */
typedef struct xtensa_win0_ctx xtensa_win0_ctx_t;

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_WIN0_H_ */
