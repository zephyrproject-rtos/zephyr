/**
 * Copyright (c) 2025, Microchip Technology Inc.
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

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following structure defines the list of registers that need to be
 * saved/restored when a cooperative context switch occurs.
 */
struct _callee_saved {
	uint32_t W8;  /* working register w8 */
	uint32_t W9;  /* working register w9 */
	uint32_t W10; /* working register w10 */
	uint32_t W11; /* working register w11 */
	uint32_t W12; /* working register w12 */
	uint32_t W13; /* working register w13 */
	uint32_t W14; /* working register w14 */

	uint32_t F8;  /* Floating point register F8 */
	uint32_t F9;  /* Floating point register F8 */
	uint32_t F10; /* Floating point register F8 */
	uint32_t F11; /* Floating point register F8 */
	uint32_t F12; /* Floating point register F8 */
	uint32_t F13; /* Floating point register F8 */
	uint32_t F14; /* Floating point register F8 */
	uint32_t F15; /* Floating point register F8 */
	uint32_t F16; /* Floating point register F8 */
	uint32_t F17; /* Floating point register F8 */
	uint32_t F18; /* Floating point register F8 */
	uint32_t F19; /* Floating point register F8 */
	uint32_t F20; /* Floating point register F8 */
	uint32_t F21; /* Floating point register F8 */
	uint32_t F22; /* Floating point register F8 */
	uint32_t F23; /* Floating point register F8 */
	uint32_t F24; /* Floating point register F8 */
	uint32_t F25; /* Floating point register F8 */
	uint32_t F26; /* Floating point register F8 */
	uint32_t F27; /* Floating point register F8 */
	uint32_t F28; /* Floating point register F8 */
	uint32_t F29; /* Floating point register F8 */
	uint32_t F30; /* Floating point register F8 */
	uint32_t F31; /* Floating point register F8 */

	uint32_t Rcount;  /* repeat loop counter register */
	uint32_t Corcon;  /* core mode control register */
	uint32_t modcon;  /* Modulo addressing control register */
	uint32_t xmodsrt; /* X AGU modulo addressing start register */
	uint32_t xmodend; /* X AGU modulo addressing end register */
	uint32_t ymodsrt; /* Y AGU modulo addressing start register */
	uint32_t ymodend; /* Y AGU modulo addressing end register */
	uint32_t Xbrev;   /*X AGU reversal addressing control register */

	uint32_t AccL; /* Lower 32 bits of accumulator A */
	uint32_t AccH; /* Higher 32 bits of accumulator A */
	uint32_t AccU; /* sign extended upper bits of Accumulator A */
	uint32_t BccL; /* Lower 32 bits of accumulator B */
	uint32_t BccH; /* Higher 32 bits of accumulator B */
	uint32_t BccU; /* sign extended upper bits of Accumulator B */

	uint32_t stack; /* stack pointer, W15 register*/
	uint32_t frame; /* Frame pointer, w14 register */
	uint32_t splim; /* stack limit register */
};
typedef struct _callee_saved _callee_saved_t;

struct _thread_arch {
	/* current cpu priority level */
	uint32_t cpu_level;

	/* stack limit value for the thread */
	uint32_t splim;

	/* return value of z_swap */
	uint32_t swap_return_value;
};
typedef struct _thread_arch _thread_arch_t;

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_THREAD_H_ */
