/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifdef CONFIG_X86_64
#include <intel64/kernel_arch_func.h>
#else
#include <ia32/kernel_arch_func.h>
#endif

#ifndef _ASMLANGUAGE
static inline bool z_arch_is_in_isr(void)
{
#ifdef CONFIG_SMP
	return z_arch_curr_cpu()->nested != 0;
#else
	return _kernel.nested != 0U;
#endif
}

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_interrupt_stack1, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_interrupt_stack2, CONFIG_ISR_STACK_SIZE);
extern K_THREAD_STACK_DEFINE(_interrupt_stack3, CONFIG_ISR_STACK_SIZE);

struct multiboot_info;

extern FUNC_NORETURN void z_x86_prep_c(int dummy, struct multiboot_info *info);

#ifdef CONFIG_X86_VERY_EARLY_CONSOLE
/* Setup ultra-minimal serial driver for printk() */
void z_x86_early_serial_init(void);
#endif /* CONFIG_X86_VERY_EARLY_CONSOLE */

#ifdef CONFIG_X86_MMU
/* Create all page tables with boot configuration and enable paging */
void z_x86_paging_init(void);
#endif /* CONFIG_X86_MMU */

#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_ */
