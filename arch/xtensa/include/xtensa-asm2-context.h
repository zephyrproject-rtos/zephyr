/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_

#include <xtensa/corebits.h>
#include <xtensa/config/core-isa.h>

/*
 * Stack frame layout for a saved processor context, in memory order,
 * high to low address:
 *
 * SP-0 <-- Interrupted stack pointer points here
 *
 * SP-4   Caller A3 spill slot \
 * SP-8   Caller A2 spill slot |
 * SP-12  Caller A1 spill slot + (Part of ABI standard)
 * SP-16  Caller A0 spill slot /
 *
 * SP-20  Saved A3
 * SP-24  Saved A2
 * SP-28  Unused (not "Saved A1" because the SP is saved externally as a handle)
 * SP-32  Saved A0
 *
 * SP-36  Saved PC (address to jump to following restore)
 * SP-40  Saved/interrupted PS special register
 *
 * SP-44  Saved SAR special register
 *
 * SP-48  Saved LBEG special register (if loops enabled)
 * SP-52  Saved LEND special register (if loops enabled)
 * SP-56  Saved LCOUNT special register (if loops enabled)
 *
 * SP-60  Saved SCOMPARE special register (if S32C1I enabled)
 *
 *       (The above fixed-size region is known as the "base save area" in the
 *        code below)
 *
 * - Saved A7 \
 * - Saved A6 |
 * - Saved A5 +- If not in-use by another frame
 * - Saved A4 /
 *
 * - Saved A11 \
 * - Saved A10 |
 * - Saved A9  +- If not in-use by another frame
 * - Saved A8  /
 *
 * - Saved A15 \
 * - Saved A14 |
 * - Saved A13 +- If not in-use by another frame
 * - Saved A12 /
 *
 * - Saved intermediate stack pointer (points to low word of base save
 *   area, i.e. the saved LCOUNT or SAR).  The pointer to this value
 *   (i.e. the final stack pointer) is stored externally as the
 *   "restore handle" in the thread context.
 *
 * Essentially, you can recover a pointer to the BSA by loading *SP.
 * Adding the fixed BSA size to that gets you back to the
 * original/interrupted stack pointer.
 */

#define BASE_SAVE_AREA_SIZE_COMMON	44
#define BASE_SAVE_AREA_SIZE_EXCCAUSE	4

#if XCHAL_HAVE_LOOPS
#define BASE_SAVE_AREA_SIZE_LOOPS	12
#else
#define BASE_SAVE_AREA_SIZE_LOOPS	0
#endif

#if XCHAL_HAVE_S32C1I
#define BASE_SAVE_AREA_SIZE_SCOMPARE	4
#else
#define BASE_SAVE_AREA_SIZE_SCOMPARE	0
#endif

#if XCHAL_HAVE_THREADPTR && defined(CONFIG_THREAD_LOCAL_STORAGE)
#define BASE_SAVE_AREA_SIZE_THREADPTR	4
#else
#define BASE_SAVE_AREA_SIZE_THREADPTR	0
#endif

#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
#define BASE_SAVE_AREA_SIZE_FPU		(18 * 4)
#else
#define BASE_SAVE_AREA_SIZE_FPU		0
#endif

#define BASE_SAVE_AREA_SIZE \
	(BASE_SAVE_AREA_SIZE_COMMON + \
	 BASE_SAVE_AREA_SIZE_LOOPS + \
	 BASE_SAVE_AREA_SIZE_EXCCAUSE + \
	 BASE_SAVE_AREA_SIZE_SCOMPARE + \
	 BASE_SAVE_AREA_SIZE_THREADPTR + \
	 BASE_SAVE_AREA_SIZE_FPU)

#define BSA_A3_OFF	(BASE_SAVE_AREA_SIZE - 20)
#define BSA_A2_OFF	(BASE_SAVE_AREA_SIZE - 24)
#define BSA_SCRATCH_OFF	(BASE_SAVE_AREA_SIZE - 28)
#define BSA_A0_OFF	(BASE_SAVE_AREA_SIZE - 32)
#define BSA_PC_OFF	(BASE_SAVE_AREA_SIZE - 36)
#define BSA_PS_OFF	(BASE_SAVE_AREA_SIZE - 40)
#define BSA_SAR_OFF	(BASE_SAVE_AREA_SIZE - 44)
#define BSA_LBEG_OFF	(BASE_SAVE_AREA_SIZE - 48)
#define BSA_LEND_OFF	(BASE_SAVE_AREA_SIZE - 52)
#define BSA_LCOUNT_OFF	(BASE_SAVE_AREA_SIZE - 56)

#define BSA_EXCCAUSE_OFF \
			(BASE_SAVE_AREA_SIZE - \
			 (BASE_SAVE_AREA_SIZE_COMMON + \
			  BASE_SAVE_AREA_SIZE_LOOPS + \
			  BASE_SAVE_AREA_SIZE_EXCCAUSE))
#if XCHAL_HAVE_S32C1I
#define BSA_SCOMPARE1_OFF \
			(BASE_SAVE_AREA_SIZE - \
			 (BASE_SAVE_AREA_SIZE_COMMON + \
			  BASE_SAVE_AREA_SIZE_LOOPS + \
			  BASE_SAVE_AREA_SIZE_EXCCAUSE + \
			  BASE_SAVE_AREA_SIZE_SCOMPARE))
#endif

#if XCHAL_HAVE_THREADPTR && defined(CONFIG_THREAD_LOCAL_STORAGE)
#define BSA_THREADPTR_OFF \
			(BASE_SAVE_AREA_SIZE - \
			 (BASE_SAVE_AREA_SIZE_COMMON + \
			  BASE_SAVE_AREA_SIZE_LOOPS + \
			  BASE_SAVE_AREA_SIZE_EXCCAUSE + \
			  BASE_SAVE_AREA_SIZE_SCOMPARE + \
			  BASE_SAVE_AREA_SIZE_THREADPTR))
#endif

#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
#define BSA_FPU_OFF \
			(BASE_SAVE_AREA_SIZE - \
			 (BASE_SAVE_AREA_SIZE_COMMON + \
			  BASE_SAVE_AREA_SIZE_LOOPS + \
			  BASE_SAVE_AREA_SIZE_EXCCAUSE + \
			  BASE_SAVE_AREA_SIZE_SCOMPARE + \
			  BASE_SAVE_AREA_SIZE_THREADPTR + \
			  BASE_SAVE_AREA_SIZE_FPU))
#endif

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_ */
