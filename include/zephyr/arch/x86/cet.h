/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_CET_H_
#define ZEPHYR_INCLUDE_ARCH_X86_CET_H_

#ifndef _ASMLANGUAGE

#ifdef CONFIG_HW_SHADOW_STACK

extern FUNC_NORETURN void z_thread_entry(k_thread_entry_t entry,
					 void *p1, void *p2, void *p3);

typedef uintptr_t arch_thread_hw_shadow_stack_t;

#define ARCH_THREAD_HW_SHADOW_STACK_SIZE(size_) \
	MAX(ROUND_UP((CONFIG_HW_SHADOW_STACK_PERCENTAGE_SIZE * (size_) / 100), \
		      CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT), \
	    CONFIG_HW_SHADOW_STACK_MIN_SIZE)

#define ARCH_THREAD_HW_SHADOW_STACK_DECLARE(sym, size) \
	extern arch_thread_hw_shadow_stack_t sym[size / sizeof(arch_thread_hw_shadow_stack_t)]

#define ARCH_THREAD_HW_SHADOW_STACK_ARRAY_DECLARE(sym, nmemb, size) \
	extern arch_thread_hw_shadow_stack_t \
		sym[nmemb][size / sizeof(arch_thread_hw_shadow_stack_t)]

#define ARCH_THREAD_HW_SHADOW_STACK_ARRAY_DEFINE(name, nmemb, size) \
	arch_thread_hw_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack.arr_ ##name) \
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT) \
	name[MAX(nmemb, 1)][size / sizeof(arch_thread_hw_shadow_stack_t)] = \
		{ \
			[0][0] = nmemb, \
		}

#ifdef CONFIG_X86_64
#define ARCH_THREAD_HW_SHADOW_STACK_DEFINE(name, size) \
	arch_thread_hw_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack) \
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT) \
	name[size / sizeof(arch_thread_hw_shadow_stack_t)] = \
		{ [size / sizeof(arch_thread_hw_shadow_stack_t) - 5] = \
		   (uintptr_t)name + size - 4 * sizeof(arch_thread_hw_shadow_stack_t) + 1, \
		  [size / sizeof(arch_thread_hw_shadow_stack_t) - 4] = \
		  (uintptr_t)name + size - 1 * sizeof(arch_thread_hw_shadow_stack_t), \
		  [size / sizeof(arch_thread_hw_shadow_stack_t) - 3] = \
		   (uintptr_t)z_thread_entry, \
		  [size / sizeof(arch_thread_hw_shadow_stack_t) - 2] = \
		   (uintptr_t)X86_KERNEL_CS }

#else /* CONFIG_X86_64 */

#ifdef CONFIG_X86_DEBUG_INFO
extern void z_x86_thread_entry_wrapper(k_thread_entry_t entry,
				      void *p1, void *p2, void *p3);
#define ___x86_entry_point z_x86_thread_entry_wrapper
#else
#define ___x86_entry_point z_thread_entry
#endif

#define ARCH_THREAD_HW_SHADOW_STACK_DEFINE(name, size) \
	arch_thread_hw_shadow_stack_t Z_GENERIC_SECTION(.x86shadowstack) \
	__aligned(CONFIG_X86_CET_SHADOW_STACK_ALIGNMENT) \
	name[size / sizeof(arch_thread_hw_shadow_stack_t)] = \
		{ [size / sizeof(arch_thread_hw_shadow_stack_t) - 4] = \
		   (uintptr_t)name + size - 2 * sizeof(arch_thread_hw_shadow_stack_t), \
		  [size / sizeof(arch_thread_hw_shadow_stack_t) - 3] = 0, \
		  [size / sizeof(arch_thread_hw_shadow_stack_t) - 2] = \
		   (uintptr_t)___x86_entry_point, \
		}
#endif /* CONFIG_X86_64 */

int arch_thread_hw_shadow_stack_attach(struct k_thread *thread,
				       arch_thread_hw_shadow_stack_t *stack,
				       size_t stack_size);

#endif /* CONFIG_HW_SHADOW_STACK */

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_X86_CET_H_ */
