/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RH850_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_RH850_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	uint32_t ctpc;
	uint32_t ctpsw;
	uint32_t eiwr;
	uint32_t eipc;
	uint32_t eipsw;
	uint32_t lp;
#ifdef CONFIG_FXU
	uint32_t fxxc;
	uint32_t fxsr;
#endif
#ifdef CONFIG_FPU_SHARING
	uint32_t fpepc;
	uint32_t fpsr;
#endif
	uint32_t r30;
	uint32_t r29;
	uint32_t r28;
	uint32_t r27;
	uint32_t r26;
	uint32_t r25;
	uint32_t r24;
	uint32_t r23;
	uint32_t r22;
	uint32_t r21;
	uint32_t r20;
	uint32_t r19;
	uint32_t r18;
	uint32_t r17;
	uint32_t r16;
	uint32_t r15;
	uint32_t r14;
	uint32_t r13;
	uint32_t r12;
	uint32_t r11;
	uint32_t r10;
	uint32_t r9;
	uint32_t r8;
	uint32_t r7;
	uint32_t r6;
	uint32_t r5;
	uint32_t r2;
	uint32_t r1;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_RH850_EXCEPTION_H_ */
