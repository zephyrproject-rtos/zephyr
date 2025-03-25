/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_CET_H_
#define ZEPHYR_INCLUDE_ARCH_X86_CET_H_

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#ifdef CONFIG_X86_CET_SHADOW_STACK

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
					 void *p1, void *p2, void *p3);

typedef long z_x86_shadow_stack_t;

#ifdef CONFIG_X86_64
#define Z_X86_SHADOW_STACK_DEFINE(name, size) \
	z_x86_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack) \
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT) \
	name[size / sizeof(z_x86_shadow_stack_t)] = \
		{ [size / sizeof(z_x86_shadow_stack_t) - 5] = \
		   (uintptr_t)name + size - 4 * sizeof(z_x86_shadow_stack_t) + 1, \
		  [size / sizeof(z_x86_shadow_stack_t) - 4] = \
		  (uintptr_t)name + size - 1 * sizeof(z_x86_shadow_stack_t), \
		  [size / sizeof(z_x86_shadow_stack_t) - 3] = \
		   (uintptr_t)z_thread_entry, \
		  [size / sizeof(z_x86_shadow_stack_t) - 2] = \
		   (uintptr_t)X86_KERNEL_CS }
#else

#ifdef CONFIG_X86_DEBUG_INFO
extern void z_x86_thread_entry_wrapper(k_thread_entry_t entry,
				      void *p1, void *p2, void *p3);
#define ___x86_entry_point z_x86_thread_entry_wrapper
#else
#define ___x86_entry_point z_thread_entry
#endif

#define Z_X86_SHADOW_STACK_DEFINE(name, size) \
	z_x86_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack) \
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT) \
	name[size / sizeof(z_x86_shadow_stack_t)] = \
		{ [size / sizeof(z_x86_shadow_stack_t) - 4] = \
		   (uintptr_t)name + size - 2 * sizeof(z_x86_shadow_stack_t), \
		  [size / sizeof(z_x86_shadow_stack_t) - 3] = 0, \
		  [size / sizeof(z_x86_shadow_stack_t) - 2] = \
		   (uintptr_t)___x86_entry_point, \
		}

#endif

int z_x86_thread_attach_shadow_stack(k_tid_t thread,
				     z_x86_shadow_stack_t *stack,
				     size_t stack_size);

#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_CET_H_ */
