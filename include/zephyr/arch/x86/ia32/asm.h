/* asm.h - x86 tool dependent headers */

/*
 * Copyright (c) 2007-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_IA32_ASM_H_
#define ZEPHYR_INCLUDE_ARCH_X86_IA32_ASM_H_

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#if defined(_ASMLANGUAGE)

#ifdef CONFIG_X86_KPTI
GTEXT(z_x86_trampoline_to_user)
GTEXT(z_x86_trampoline_to_kernel)

#define KPTI_IRET	jmp z_x86_trampoline_to_user
#define KPTI_IRET_USER	jmp z_x86_trampoline_to_user_always
#else
#define KPTI_IRET	iret
#define KPTI_IRET_USER	iret
#endif /* CONFIG_X86_KPTI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_IA32_ASM_H_ */
