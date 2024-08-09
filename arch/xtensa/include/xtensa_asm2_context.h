/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_
#define ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_

#if defined(__XT_CLANG__)
#include <xtensa/xtensa-types.h>
#endif

#include <xtensa/corebits.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/config/tie.h>

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
 * SP-64  Saved EXCCAUSE special register
 *
 * SP-68  Saved THREADPTR special register (if processor has thread pointer)
 *
 *       (The above fixed-size region is known as the "base save area" in the
 *        code below)
 *
 * - 18 FPU registers (if FPU is present and CONFIG_FPU_SHARING enabled)
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

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <zephyr/toolchain.h>

/**
 * Base Save Area (BSA) during interrupt.
 *
 * This saves the registers during interrupt entrance
 * so they can be restored later.
 *
 * Note that only A0-A3 are saved here. High registers
 * are saved after the BSA.
 */
struct xtensa_irq_base_save_area {
#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
	uintptr_t fcr;
	uintptr_t fsr;
	uintptr_t fpu0;
	uintptr_t fpu1;
	uintptr_t fpu2;
	uintptr_t fpu3;
	uintptr_t fpu4;
	uintptr_t fpu5;
	uintptr_t fpu6;
	uintptr_t fpu7;
	uintptr_t fpu8;
	uintptr_t fpu9;
	uintptr_t fpu10;
	uintptr_t fpu11;
	uintptr_t fpu12;
	uintptr_t fpu13;
	uintptr_t fpu14;
	uintptr_t fpu15;
#endif

#if defined(CONFIG_XTENSA_HIFI_SHARING)

	/*
	 * Carve space for the registers used by the HiFi audio engine
	 * coprocessor (which is always CP1). Carve additional space to
	 * manage alignment at run-time as we can not yet guarantee the
	 * alignment of the BSA.
	 */

	uint8_t  hifi[XCHAL_CP1_SA_SIZE + XCHAL_CP1_SA_ALIGN];
#endif

#if XCHAL_HAVE_THREADPTR
	uintptr_t threadptr;
#endif

#if XCHAL_HAVE_S32C1I
	uintptr_t scompare1;
#endif

	uintptr_t exccause;

#if XCHAL_HAVE_LOOPS
	uintptr_t lcount;
	uintptr_t lend;
	uintptr_t lbeg;
#endif

	uintptr_t sar;
	uintptr_t ps;
	uintptr_t pc;
	uintptr_t a0;
	uintptr_t scratch;
	uintptr_t a2;
	uintptr_t a3;

	uintptr_t caller_a0;
	uintptr_t caller_a1;
	uintptr_t caller_a2;
	uintptr_t caller_a3;
};

typedef struct xtensa_irq_base_save_area _xtensa_irq_bsa_t;

/**
 * Raw interrupt stack frame.
 *
 * This provides a raw interrupt stack frame to make it
 * easier to construct general purpose code in loops.
 * Avoid using this if possible.
 */
struct xtensa_irq_stack_frame_raw {
	_xtensa_irq_bsa_t *ptr_to_bsa;

	struct {
		uintptr_t r0;
		uintptr_t r1;
		uintptr_t r2;
		uintptr_t r3;
	} blks[3];
};

typedef struct xtensa_irq_stack_frame_raw _xtensa_irq_stack_frame_raw_t;

/**
 * Interrupt stack frame containing A0 - A15.
 */
struct xtensa_irq_stack_frame_a15 {
	_xtensa_irq_bsa_t *ptr_to_bsa;

	uintptr_t a12;
	uintptr_t a13;
	uintptr_t a14;
	uintptr_t a15;

	uintptr_t a8;
	uintptr_t a9;
	uintptr_t a10;
	uintptr_t a11;

	uintptr_t a4;
	uintptr_t a5;
	uintptr_t a6;
	uintptr_t a7;

	_xtensa_irq_bsa_t bsa;
};

typedef struct xtensa_irq_stack_frame_a15 _xtensa_irq_stack_frame_a15_t;

/**
 * Interrupt stack frame containing A0 - A11.
 */
struct xtensa_irq_stack_frame_a11 {
	_xtensa_irq_bsa_t *ptr_to_bsa;

	uintptr_t a8;
	uintptr_t a9;
	uintptr_t a10;
	uintptr_t a11;

	uintptr_t a4;
	uintptr_t a5;
	uintptr_t a6;
	uintptr_t a7;

	_xtensa_irq_bsa_t bsa;
};

typedef struct xtensa_irq_stack_frame_a11 _xtensa_irq_stack_frame_a11_t;

/**
 * Interrupt stack frame containing A0 - A7.
 */
struct xtensa_irq_stack_frame_a7 {
	_xtensa_irq_bsa_t *ptr_to_bsa;

	uintptr_t a4;
	uintptr_t a5;
	uintptr_t a6;
	uintptr_t a7;

	_xtensa_irq_bsa_t bsa;
};

typedef struct xtensa_irq_stack_frame_a7 _xtensa_irq_stack_frame_a7_t;

/**
 * Interrupt stack frame containing A0 - A3.
 */
struct xtensa_irq_stack_frame_a3 {
	_xtensa_irq_bsa_t *ptr_to_bsa;

	_xtensa_irq_bsa_t bsa;
};

typedef struct xtensa_irq_stack_frame_a3 _xtensa_irq_stack_frame_a3_t;

#endif /* __ASSEMBLER__ */

#endif /* ZEPHYR_ARCH_XTENSA_INCLUDE_XTENSA_ASM2_CONTEXT_H_ */
