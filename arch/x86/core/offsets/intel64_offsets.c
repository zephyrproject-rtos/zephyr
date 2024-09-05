/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _X86_OFFSETS_INC_
#define _X86_OFFSETS_INC_

GEN_OFFSET_STRUCT(arch_csf, rsp);
GEN_OFFSET_STRUCT(arch_csf, rbp);
GEN_OFFSET_STRUCT(arch_csf, rbx);
GEN_OFFSET_STRUCT(arch_csf, r12);
GEN_OFFSET_STRUCT(arch_csf, r13);
GEN_OFFSET_STRUCT(arch_csf, r14);
GEN_OFFSET_STRUCT(arch_csf, r15);
GEN_OFFSET_STRUCT(arch_csf, rip);
GEN_OFFSET_STRUCT(arch_csf, rflags);

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

GEN_OFFSET_SYM(x86_tss64_t, ist1);
GEN_OFFSET_SYM(x86_tss64_t, ist2);
GEN_OFFSET_SYM(x86_tss64_t, ist6);
GEN_OFFSET_SYM(x86_tss64_t, ist7);
GEN_OFFSET_SYM(x86_tss64_t, cpu);
#ifdef CONFIG_USERSPACE
GEN_OFFSET_SYM(x86_tss64_t, psp);
GEN_OFFSET_SYM(x86_tss64_t, usp);
#endif /* CONFIG_USERSPACE */
GEN_ABSOLUTE_SYM(__X86_TSS64_SIZEOF, sizeof(x86_tss64_t));

GEN_OFFSET_SYM(x86_cpuboot_t, tr);
GEN_OFFSET_SYM(x86_cpuboot_t, gs_base);
GEN_OFFSET_SYM(x86_cpuboot_t, sp);
GEN_OFFSET_SYM(x86_cpuboot_t, stack_size);
GEN_ABSOLUTE_SYM(__X86_CPUBOOT_SIZEOF, sizeof(x86_cpuboot_t));

#endif /* _X86_OFFSETS_INC_ */
