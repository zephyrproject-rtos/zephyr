/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_ARCH_ARC_V2_SJLI_H
#define ZEPHYR_INCLUDE_ARCH_ARC_V2_SJLI_H

#define SJLI_CALL_ARC_SECURE	0

#define ARC_S_CALL_AUX_READ		0
#define ARC_S_CALL_AUX_WRITE	1
#define ARC_S_CALL_IRQ_ALLOC	2
#define ARC_S_CALL_CLRI			3
#define ARC_S_CALL_SETI			4
#define ARC_S_CALL_LIMIT		5



#define ARC_N_IRQ_START_LEVEL ((CONFIG_NUM_IRQ_PRIO_LEVELS + 1) / 2)

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stdbool.h>

#include <arch/arc/v2/aux_regs.h>

#ifdef __cplusplus
extern "C" {
#endif


#define arc_sjli(id)	\
		(__asm__ volatile("sjli %[sjli_id]\n" :: [sjli_id] "i" (id)))

#ifdef CONFIG_ARC_SECURE_FIRMWARE
typedef uint32_t (*_arc_s_call_handler_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3,
				      uint32_t arg4, uint32_t arg5, uint32_t arg6);


extern void arc_go_to_normal(uint32_t addr);
extern void _arc_do_secure_call(void);
extern const _arc_s_call_handler_t arc_s_call_table[ARC_S_CALL_LIMIT];

#endif


#ifdef CONFIG_ARC_NORMAL_FIRMWARE

static inline uint32_t _arc_s_call_invoke6(uint32_t arg1, uint32_t arg2, uint32_t arg3,
					  uint32_t arg4, uint32_t arg5, uint32_t arg6,
					  uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r4 __asm__("r4") = arg5;
	register uint32_t r5 __asm__("r5") = arg6;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r5), "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke5(uint32_t arg1, uint32_t arg2, uint32_t arg3,
					  uint32_t arg4, uint32_t arg5, uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r4 __asm__("r4") = arg5;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r4), "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke4(uint32_t arg1, uint32_t arg2, uint32_t arg3,
					  uint32_t arg4, uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r3 __asm__("r3") = arg4;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r3),
			   "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke3(uint32_t arg1, uint32_t arg2, uint32_t arg3,
					  uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r2 __asm__("r2") = arg3;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r2), "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke2(uint32_t arg1, uint32_t arg2, uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r1 __asm__("r1") = arg2;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r1), "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke1(uint32_t arg1, uint32_t call_id)
{
	register uint32_t ret __asm__("r0") = arg1;
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline uint32_t _arc_s_call_invoke0(uint32_t call_id)
{
	register uint32_t ret __asm__("r0");
	register uint32_t r6 __asm__("r6") = call_id;

	compiler_barrier();

	__asm__ volatile(
			 "sjli %[id]\n"
			 : "=r"(ret)
			 : [id] "i" (SJLI_CALL_ARC_SECURE),
			   "r" (ret), "r" (r6));

	return ret;
}

static inline bool _arch_is_user_context(void)
{
	uint32_t status;

	compiler_barrier();

	__asm__ volatile("lr %0, [%[status32]]\n"
			 : "=r"(status)
			 : [status32] "i" (_ARC_V2_STATUS32));

	return !(status & _ARC_V2_STATUS32_US) ? true : false;
}




#endif /* CONFIG_ARC_NORMAL_FIRMWARE */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_V2_SECURE_H */
