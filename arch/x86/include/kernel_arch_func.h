/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifdef CONFIG_X86_LONGMODE
#include <intel64/kernel_arch_func.h>
#else
#include <ia32/kernel_arch_func.h>
#endif

#define z_is_in_isr() (_kernel.nested != 0U)

#ifndef _ASMLANGUAGE

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_ */
