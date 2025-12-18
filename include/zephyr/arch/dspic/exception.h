/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief dsPIC33A exception stack frame definition
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Exception stack frame for dsPIC33A.
 *
 * Holds the caller-saved registers pushed onto the stack when an
 * interrupt or exception is taken.
 */
struct arch_esf {
	/** Status register */
	uint32_t SR;
	/** Program counter */
	uint32_t PC;
	/** Repeat loop counter register */
	uint32_t RCOUNT;
	/** Floating-point status register */
	uint32_t FSR;
	/** Floating-point control register */
	uint32_t FCR;
	/** Working register W0 */
	uint32_t W0;
	/** Working register W1 */
	uint32_t W1;
	/** Working register W2 */
	uint32_t W2;
	/** Working register W3 */
	uint32_t W3;
	/** Working register W4 */
	uint32_t W4;
	/** Working register W5 */
	uint32_t W5;
	/** Working register W6 */
	uint32_t W6;
	/** Working register W7 */
	uint32_t W7;
	/** Floating-point register F0 */
	uint32_t F0;
	/** Floating-point register F1 */
	uint32_t F1;
	/** Floating-point register F2 */
	uint32_t F2;
	/** Floating-point register F3 */
	uint32_t F3;
	/** Floating-point register F4 */
	uint32_t F4;
	/** Floating-point register F5 */
	uint32_t F5;
	/** Floating-point register F6 */
	uint32_t F6;
	/** Floating-point register F7 */
	uint32_t F7;
	/** Previous frame pointer (W14) */
	uint32_t FRAME;
};

/** @brief Typedef for the dsPIC33A exception stack frame structure */
typedef struct arch_esf arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_ */
