/**
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief dsPIC33A per-architecture thread definitions
 *
 * This file contains definitions for:
 *  - struct _thread_arch
 *  - struct _callee_saved
 *
 * These are necessary to instantiate instances of struct k_thread.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callee-saved register context for dsPIC33A.
 *
 * Holds all registers that must be preserved across a cooperative
 * context switch. Saved and restored by the context-switch assembly
 * routines in sequential order.
 */
struct _callee_saved {
	/** Working register w8 */
	uint32_t w8;
	/** Working register w9 */
	uint32_t w9;
	/** Working register w10 */
	uint32_t w10;
	/** Working register w11 */
	uint32_t w11;
	/** Working register w12 */
	uint32_t w12;
	/** Working register w13 */
	uint32_t w13;
	/** Working register w14 (frame pointer) */
	uint32_t w14;

	/** Floating-point register f8 */
	uint32_t f8;
	/** Floating-point register f9 */
	uint32_t f9;
	/** Floating-point register f10 */
	uint32_t f10;
	/** Floating-point register f11 */
	uint32_t f11;
	/** Floating-point register f12 */
	uint32_t f12;
	/** Floating-point register f13 */
	uint32_t f13;
	/** Floating-point register f14 */
	uint32_t f14;
	/** Floating-point register f15 */
	uint32_t f15;
	/** Floating-point register f16 */
	uint32_t f16;
	/** Floating-point register f17 */
	uint32_t f17;
	/** Floating-point register f18 */
	uint32_t f18;
	/** Floating-point register f19 */
	uint32_t f19;
	/** Floating-point register f20 */
	uint32_t f20;
	/** Floating-point register f21 */
	uint32_t f21;
	/** Floating-point register f22 */
	uint32_t f22;
	/** Floating-point register f23 */
	uint32_t f23;
	/** Floating-point register f24 */
	uint32_t f24;
	/** Floating-point register f25 */
	uint32_t f25;
	/** Floating-point register f26 */
	uint32_t f26;
	/** Floating-point register f27 */
	uint32_t f27;
	/** Floating-point register f28 */
	uint32_t f28;
	/** Floating-point register f29 */
	uint32_t f29;
	/** Floating-point register f30 */
	uint32_t f30;
	/** Floating-point register f31 */
	uint32_t f31;

	/** Repeat loop counter register (RCOUNT) */
	uint32_t rcount;
	/** Core control register (CORCON) */
	uint32_t corcon;
	/** X AGU bit-reversal addressing control register (XBREV) */
	uint32_t xbrev;

	/** Lower 32 bits of DSP accumulator A */
	uint32_t acc_l;
	/** Upper 32 bits of DSP accumulator A */
	uint32_t acc_h;
	/** Sign-extended guard bits of DSP accumulator A */
	uint32_t acc_u;
	/** Lower 32 bits of DSP accumulator B */
	uint32_t bcc_l;
	/** Upper 32 bits of DSP accumulator B */
	uint32_t bcc_h;
	/** Sign-extended guard bits of DSP accumulator B */
	uint32_t bcc_u;

	/** Stack pointer limit register (SPLIM) */
	uint32_t splim;
	/** Stack pointer (W15) */
	uint32_t stack;
	/** Frame pointer (W14) */
	uint32_t frame;
};

/** @brief Typedef for the callee-saved register context structure */
typedef struct _callee_saved _callee_saved_t;

/**
 * @brief Architecture-specific thread state for dsPIC33A.
 */
struct _thread_arch {
	/** Current CPU interrupt priority level for this thread */
	uint32_t cpu_level;

	/** Stack pointer limit (SPLIM) value for this thread */
	uint32_t splim;

	/** Return value to restore after a context switch */
	uint32_t switch_return_value;

	/** Non-zero when the thread was switched out and needs return value restored */
	uint32_t switched_from_thread;
};

/** @brief Typedef for the architecture-specific thread state structure */
typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_ */
