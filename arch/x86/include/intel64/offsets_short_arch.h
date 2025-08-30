/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_

#include <zephyr/offsets.h>

#define _thread_offset_to_rsp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_rsp_OFFSET)

#define _thread_offset_to_rbx \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_rbx_OFFSET)

#define _thread_offset_to_rbp \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_rbp_OFFSET)

#define _thread_offset_to_r12 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r12_OFFSET)

#define _thread_offset_to_r13 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r13_OFFSET)

#define _thread_offset_to_r14 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r14_OFFSET)

#define _thread_offset_to_r15 \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_r15_OFFSET)

#define _thread_offset_to_rip \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_rip_OFFSET)

#define _thread_offset_to_rflags \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_rflags_OFFSET)

#define _thread_offset_to_rax \
	(___thread_t_arch_OFFSET + ___thread_arch_t_rax_OFFSET)

#define _thread_offset_to_rcx \
	(___thread_t_arch_OFFSET + ___thread_arch_t_rcx_OFFSET)

#define _thread_offset_to_rdx \
	(___thread_t_arch_OFFSET + ___thread_arch_t_rdx_OFFSET)

#define _thread_offset_to_rsi \
	(___thread_t_arch_OFFSET + ___thread_arch_t_rsi_OFFSET)

#define _thread_offset_to_rdi \
	(___thread_t_arch_OFFSET + ___thread_arch_t_rdi_OFFSET)

#define _thread_offset_to_r8 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_r8_OFFSET)

#define _thread_offset_to_r9 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_r9_OFFSET)

#define _thread_offset_to_r10 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_r10_OFFSET)

#define _thread_offset_to_r11 \
	(___thread_t_arch_OFFSET + ___thread_arch_t_r11_OFFSET)

#define _thread_offset_to_sse \
	(___thread_t_arch_OFFSET + ___thread_arch_t_sse_OFFSET)

#define _thread_offset_to_ss \
	(___thread_t_arch_OFFSET + ___thread_arch_t_ss_OFFSET)

#define _thread_offset_to_cs \
	(___thread_t_arch_OFFSET + ___thread_arch_t_cs_OFFSET)

#ifdef CONFIG_HW_SHADOW_STACK
#define _thread_offset_to_shstk_addr \
	(___thread_t_arch_OFFSET + ___thread_arch_t_shstk_addr_OFFSET)

#define _thread_offset_to_shstk_base \
	(___thread_t_arch_OFFSET + ___thread_arch_t_shstk_base_OFFSET)

#define _thread_offset_to_shstk_size \
	(___thread_t_arch_OFFSET + ___thread_arch_t_shstk_size_OFFSET)
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_ */
