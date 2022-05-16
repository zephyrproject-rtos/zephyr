/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

/* kernel */

/* nothing for now */

/* end - kernel */

/* threads */

#define _thread_offset_to_sp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

#define _thread_offset_to_ra \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_ra_OFFSET)

#define _thread_offset_to_tp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_tp_OFFSET)

#define _thread_offset_to_s0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s0_OFFSET)

#define _thread_offset_to_s1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s1_OFFSET)

#define _thread_offset_to_s2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s2_OFFSET)

#define _thread_offset_to_s3 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s3_OFFSET)

#define _thread_offset_to_s4 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s4_OFFSET)

#define _thread_offset_to_s5 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s5_OFFSET)

#define _thread_offset_to_s6 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s6_OFFSET)

#define _thread_offset_to_s7 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s7_OFFSET)

#define _thread_offset_to_s8 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s8_OFFSET)

#define _thread_offset_to_s9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s9_OFFSET)

#define _thread_offset_to_s10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s10_OFFSET)

#define _thread_offset_to_s11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_s11_OFFSET)

#define _thread_offset_to_swap_return_value \
	(___thread_t_arch_OFFSET + ___thread_arch_t_swap_return_value_OFFSET)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)

#define _thread_offset_to_fcsr \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fcsr_OFFSET)

#define _thread_offset_to_fs0 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs0_OFFSET)

#define _thread_offset_to_fs1 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs1_OFFSET)

#define _thread_offset_to_fs2 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs2_OFFSET)

#define _thread_offset_to_fs3 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs3_OFFSET)

#define _thread_offset_to_fs4 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs4_OFFSET)

#define _thread_offset_to_fs5 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs5_OFFSET)

#define _thread_offset_to_fs6 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs6_OFFSET)

#define _thread_offset_to_fs7 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs7_OFFSET)

#define _thread_offset_to_fs8 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs8_OFFSET)

#define _thread_offset_to_fs9 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs9_OFFSET)

#define _thread_offset_to_fs10 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs10_OFFSET)

#define _thread_offset_to_fs11 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_fs11_OFFSET)

#endif /* defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING) */

#ifdef CONFIG_USERSPACE
#define _thread_offset_to_priv_stack_start \
	(___thread_t_arch_OFFSET + ___thread_arch_t_priv_stack_start_OFFSET)
#define _thread_offset_to_user_sp \
	(___thread_t_arch_OFFSET + ___thread_arch_t_user_sp_OFFSET)
#endif

/* end - threads */

#endif /* ZEPHYR_ARCH_RISCV_INCLUDE_OFFSETS_SHORT_ARCH_H_ */
