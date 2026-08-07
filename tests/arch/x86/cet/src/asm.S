/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/x86/ia32/asm.h>

GTEXT(should_work)
GTEXT(should_not_work)

#if defined(CONFIG_X86_64)
should_work:
    endbr64
    mov %rdi, %rax
    inc %rax
    ret

should_not_work:
    mov %rdi, %rax
    inc %rax
    ret
#else
should_work:
    endbr32
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax
    inc %eax
    pop %ebp
    ret

should_not_work:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax
    inc %eax
    pop %ebp
    ret
#endif
