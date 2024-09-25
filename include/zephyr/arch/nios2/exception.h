/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_NIOS2_EXPCEPTION_H_
#define ZEPHYR_INCLUDE_ARCH_NIOS2_EXPCEPTION_H_

#ifndef _ASMLANGUAGE
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct arch_esf {
	uint32_t ra; /* return address r31 */
	uint32_t r1; /* at */
	uint32_t r2; /* return value */
	uint32_t r3; /* return value */
	uint32_t r4; /* register args */
	uint32_t r5; /* register args */
	uint32_t r6; /* register args */
	uint32_t r7; /* register args */
	uint32_t r8; /* Caller-saved general purpose */
	uint32_t r9; /* Caller-saved general purpose */
	uint32_t r10; /* Caller-saved general purpose */
	uint32_t r11; /* Caller-saved general purpose */
	uint32_t r12; /* Caller-saved general purpose */
	uint32_t r13; /* Caller-saved general purpose */
	uint32_t r14; /* Caller-saved general purpose */
	uint32_t r15; /* Caller-saved general purpose */
	uint32_t estatus;
	uint32_t instr; /* Instruction being executed when exc occurred */
};

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_NIOS2_EXPCEPTION_H_ */
