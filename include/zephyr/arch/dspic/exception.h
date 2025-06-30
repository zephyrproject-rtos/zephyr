/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	uint32_t PC;     /* Program counter*/
	uint32_t FRAME;  /* Previous frame pointer*/
	uint32_t RCOUNT; /* repeat count register*/
	uint32_t FSR;    /* Floating point status register */
	uint32_t FCR;    /* Floating point control register*/
	uint32_t W0;     /* working register W0 */
	uint32_t W1;     /* working register W0 */
	uint32_t W2;     /* working register W0 */
	uint32_t W3;     /* working register W0 */
	uint32_t W4;     /* working register W0 */
	uint32_t W5;     /* working register W0 */
	uint32_t W6;     /* working register W0 */
	uint32_t W7;     /* working register W0 */
	uint32_t F0;     /* Floating point register F0 */
	uint32_t F1;     /* Floating point register F1 */
	uint32_t F2;     /* Floating point register F2 */
	uint32_t F3;     /* Floating point register F3 */
	uint32_t F4;     /* Floating point register F4 */
	uint32_t F5;     /* Floating point register F5 */
	uint32_t F6;     /* Floating point register F6 */
	uint32_t F7;     /* Floating point register F7 */
};
typedef struct arch_esf arch_esf_t;

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_DSPIC_EXCEPTION_H_ */
