/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _X86_OFFSETS_INC_
#define _X86_OFFSETS_INC_

GEN_OFFSET_SYM(_thread_arch_t, rax);
GEN_OFFSET_SYM(_thread_arch_t, rcx);
GEN_OFFSET_SYM(_thread_arch_t, rdx);
GEN_OFFSET_SYM(_thread_arch_t, rsi);
GEN_OFFSET_SYM(_thread_arch_t, rdi);
GEN_OFFSET_SYM(_thread_arch_t, r8);
GEN_OFFSET_SYM(_thread_arch_t, r9);
GEN_OFFSET_SYM(_thread_arch_t, r10);
GEN_OFFSET_SYM(_thread_arch_t, r11);
GEN_OFFSET_SYM(_thread_arch_t, sse);
#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(_thread_arch_t, ss);
GEN_OFFSET_SYM(_thread_arch_t, cs);
GEN_OFFSET_SYM(_thread_arch_t, psp);
#ifndef CONFIG_X86_COMMON_PAGE_TABLE
GEN_OFFSET_SYM(_thread_arch_t, ptables);
#endif
#endif /* CONFIG_USERSPACE */

#endif /* _X86_OFFSETS_INC_ */
