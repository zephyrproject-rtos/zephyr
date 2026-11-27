/*
 * Copyright (c) 2026 Jiaming Zhao
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/arm-smccc.h>

#define SMCCC_CALL(op)                                                                          \
	__asm__ volatile(op " #0"                                                               \
			 : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3),                              \
			   "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7)                               \
			 :                                                                      \
			 : "memory", "cc")

void arm_smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res)
{
	register unsigned long r0 __asm__("r0") = a0;
	register unsigned long r1 __asm__("r1") = a1;
	register unsigned long r2 __asm__("r2") = a2;
	register unsigned long r3 __asm__("r3") = a3;
	register unsigned long r4 __asm__("r4") = a4;
	register unsigned long r5 __asm__("r5") = a5;
	register unsigned long r6 __asm__("r6") = a6;
	register unsigned long r7 __asm__("r7") = a7;

	SMCCC_CALL("smc");

	res->a0 = r0;
	res->a1 = r1;
	res->a2 = r2;
	res->a3 = r3;
	res->a4 = r4;
	res->a5 = r5;
	res->a6 = r6;
	res->a7 = r7;
}

void arm_smccc_hvc(unsigned long a0, unsigned long a1, unsigned long a2, unsigned long a3,
		   unsigned long a4, unsigned long a5, unsigned long a6, unsigned long a7,
		   struct arm_smccc_res *res)
{
	register unsigned long r0 __asm__("r0") = a0;
	register unsigned long r1 __asm__("r1") = a1;
	register unsigned long r2 __asm__("r2") = a2;
	register unsigned long r3 __asm__("r3") = a3;
	register unsigned long r4 __asm__("r4") = a4;
	register unsigned long r5 __asm__("r5") = a5;
	register unsigned long r6 __asm__("r6") = a6;
	register unsigned long r7 __asm__("r7") = a7;

	SMCCC_CALL("hvc");

	res->a0 = r0;
	res->a1 = r1;
	res->a2 = r2;
	res->a3 = r3;
	res->a4 = r4;
	res->a5 = r5;
	res->a6 = r6;
	res->a7 = r7;
}
