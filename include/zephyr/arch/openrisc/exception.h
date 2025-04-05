/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_OPENRISC_EXCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_OPENRISC_EXCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	uint32_t r3;	/* Function argument */
	uint32_t r4;	/* Function argument */
	uint32_t r5;	/* Function argument */
	uint32_t r6;	/* Function argument */
	uint32_t r7;	/* Function argument */
	uint32_t r8;	/* Function argument */

	uint32_t r11;	/* Return value (low) */
	uint32_t r12;	/* Return value (high) */

	uint32_t r13;	/* Caller-saved general purpose */
	uint32_t r15;	/* Caller-saved general purpose */
	uint32_t r17;	/* Caller-saved general purpose */
	uint32_t r19;	/* Caller-saved general purpose */
	uint32_t r21;	/* Caller-saved general purpose */
	uint32_t r23;	/* Caller-saved general purpose */
	uint32_t r25;	/* Caller-saved general purpose */
	uint32_t r27;	/* Caller-saved general purpose */
	uint32_t r29;	/* Caller-saved general purpose */
	uint32_t r31;	/* Caller-saved general purpose */

	uint32_t mac_lo;
	uint32_t mac_hi;

	uint32_t epcr;
	uint32_t esr;
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_OPENRISC_EXCEPTION_H_ */
