/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_arch_thread.h>

#include <kernel_offsets.h>

GEN_OFFSET_SYM(_callee_saved_t, rsp);
GEN_OFFSET_SYM(_callee_saved_t, rbp);
GEN_OFFSET_SYM(_callee_saved_t, rbx);
GEN_OFFSET_SYM(_callee_saved_t, r12);
GEN_OFFSET_SYM(_callee_saved_t, r13);
GEN_OFFSET_SYM(_callee_saved_t, r14);
GEN_OFFSET_SYM(_callee_saved_t, r15);
GEN_OFFSET_SYM(_callee_saved_t, rip);
GEN_OFFSET_SYM(_callee_saved_t, rflags);
GEN_OFFSET_SYM(_callee_saved_t, rax);

GEN_OFFSET_SYM(_thread_arch_t, rcx);
GEN_OFFSET_SYM(_thread_arch_t, rdx);
GEN_OFFSET_SYM(_thread_arch_t, rsi);
GEN_OFFSET_SYM(_thread_arch_t, rdi);
GEN_OFFSET_SYM(_thread_arch_t, r8);
GEN_OFFSET_SYM(_thread_arch_t, r9);
GEN_OFFSET_SYM(_thread_arch_t, r10);
GEN_OFFSET_SYM(_thread_arch_t, r11);
GEN_OFFSET_SYM(_thread_arch_t, sse);
