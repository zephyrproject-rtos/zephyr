/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>
#include <kernel_offsets.h>
#include <zephyr/arch/xtensa/thread.h>

#include <xtensa_asm2_context.h>

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

#if defined(CONFIG_XTENSA_HIFI_SHARING)
GEN_OFFSET_SYM(_xtensa_irq_bsa_t, hifi);
#endif

#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(_thread_arch_t, psp);
GEN_OFFSET_SYM(_thread_arch_t, return_ps);
GEN_OFFSET_SYM(_thread_t, switch_handle);
#ifdef CONFIG_XTENSA_MMU
GEN_OFFSET_SYM(_thread_arch_t, ptables);

GEN_OFFSET_SYM(_thread_t, mem_domain_info);
GEN_OFFSET_SYM(_mem_domain_info_t, mem_domain);
GEN_OFFSET_SYM(k_mem_domain_t, arch);

GEN_OFFSET_SYM(arch_mem_domain_t, reg_asid);
GEN_OFFSET_SYM(arch_mem_domain_t, reg_ptevaddr);
GEN_OFFSET_SYM(arch_mem_domain_t, reg_ptepin_as);
GEN_OFFSET_SYM(arch_mem_domain_t, reg_ptepin_at);
GEN_OFFSET_SYM(arch_mem_domain_t, reg_vecpin_as);
GEN_OFFSET_SYM(arch_mem_domain_t, reg_vecpin_at);

#endif
#ifdef CONFIG_XTENSA_MPU
GEN_OFFSET_SYM(_thread_arch_t, mpu_map);
#endif
#endif


GEN_ABS_SYM_END
