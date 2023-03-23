/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_

#include <offsets.h>

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

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_OFFSETS_SHORT_ARCH_H_ */
