/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/x86/ia32/asm.h>

GTEXT(should_work)
GTEXT(should_not_work)

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
