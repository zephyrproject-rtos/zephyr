/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>
#include <kernel_offsets.h>

#include <xtensa-asm2-context.h>
#include <zephyr/arch/xtensa/xtensa-win0.h>

GEN_ABSOLUTE_SYM(___xtensa_irq_bsa_t_SIZEOF, sizeof(_xtensa_irq_bsa_t));
GEN_ABSOLUTE_SYM(___xtensa_irq_stack_frame_raw_t_SIZEOF, sizeof(_xtensa_irq_stack_frame_raw_t));
GEN_ABSOLUTE_SYM(___xtensa_irq_stack_frame_a15_t_SIZEOF, sizeof(_xtensa_irq_stack_frame_a15_t));
GEN_ABSOLUTE_SYM(___xtensa_irq_stack_frame_a11_t_SIZEOF, sizeof(_xtensa_irq_stack_frame_a11_t));
GEN_ABSOLUTE_SYM(___xtensa_irq_stack_frame_a7_t_SIZEOF, sizeof(_xtensa_irq_stack_frame_a7_t));
GEN_ABSOLUTE_SYM(___xtensa_irq_stack_frame_a3_t_SIZEOF, sizeof(_xtensa_irq_stack_frame_a3_t));

GEN_OFFSET_SYM(_xtensa_irq_bsa_t, a0);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, scratch);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, a2);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, a3);

GEN_OFFSET_SYM(_xtensa_irq_bsa_t, exccause);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, pc);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, ps);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, sar);

#if XCHAL_HAVE_LOOPS
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, lcount);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, lbeg);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, lend);
#endif

#if XCHAL_HAVE_S32C1I
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, scompare1);
#endif

#if XCHAL_HAVE_THREADPTR
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, threadptr);
#endif

#if XCHAL_HAVE_FP && defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_FPU_SHARING)
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fcr);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fsr);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu0);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu1);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu2);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu3);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu4);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu5);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu6);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu7);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu8);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu9);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu10);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu11);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu12);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu13);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu14);
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, fpu15);
#endif

GEN_OFFSET_SYM(xtensa_win0_ctx_t, a0);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a1);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a2);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a3);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a4);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a5);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a6);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a7);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a8);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a9);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a10);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a11);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a12);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a13);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a14);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, a15);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, ps);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, pc);
GEN_OFFSET_SYM(xtensa_win0_ctx_t, sar);
#if XCHAL_HAVE_LOOPS
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, lcount);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, lend);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, lbeg);
#endif
#if XCHAL_HAVE_S32C1I
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, scompare1);
#endif
#if XCHAL_HAVE_THREADPTR
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, threadptr);
#endif
#ifdef CONFIG_FPU_SHARING
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, fcr);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, fsr);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, fregs[16]);
#endif
/* #ifdef CONFIG_USERSPACE */
 /* Always enabled currently to support a syscall mocking layer */
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, user_a0);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, user_a1);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, user_pc);
 GEN_OFFSET_SYM(xtensa_win0_ctx_t, user_ps);
/* #endif */

GEN_ABSOLUTE_SYM(__xtensa_win0_ctx_t_SIZEOF, sizeof(xtensa_win0_ctx_t));

GEN_ABS_SYM_END
