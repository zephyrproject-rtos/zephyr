/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_H_
#define ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_H_

#if !defined(_ASMLANGUAGE)

#include <string.h>
#include <sys/arch_interface.h>

#include <kernel_structs.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_USE_SWITCH
/* This is a arch function traditionally, but when the switch-based
 * z_swap() is in use it's a simple inline provided by the kernel.
 */
static ALWAYS_INLINE void
z_arch_thread_return_value_set(struct k_thread *thread, unsigned int value)
{
	thread->swap_retval = value;
}
#endif

static ALWAYS_INLINE void
z_thread_return_value_set_with_data(struct k_thread *thread,
				   unsigned int value,
				   void *data)
{
	z_arch_thread_return_value_set(thread, value);
	thread->base.swap_data = data;
}

extern void z_init_thread_base(struct _thread_base *thread_base,
			      int priority, u32_t initial_state,
			      unsigned int options);

static ALWAYS_INLINE void z_new_thread_init(struct k_thread *thread,
					    char *pStack, size_t stackSize,
					    int prio, unsigned int options)
{
#if !defined(CONFIG_INIT_STACKS) && !defined(CONFIG_THREAD_STACK_INFO)
	ARG_UNUSED(pStack);
	ARG_UNUSED(stackSize);
#endif

#ifdef CONFIG_INIT_STACKS
	memset(pStack, 0xaa, stackSize);
#endif
#ifdef CONFIG_STACK_SENTINEL
	/* Put the stack sentinel at the lowest 4 bytes of the stack area.
	 * We periodically check that it's still present and kill the thread
	 * if it isn't.
	 */
	*((u32_t *)pStack) = STACK_SENTINEL;
#endif /* CONFIG_STACK_SENTINEL */
	/* Initialize various struct k_thread members */
	z_init_thread_base(&thread->base, prio, _THREAD_PRESTART, options);

	/* static threads overwrite it afterwards with real value */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	thread->custom_data = NULL;
#endif

#ifdef CONFIG_THREAD_NAME
	thread->name[0] = '\0';
#endif

#if defined(CONFIG_USERSPACE)
	thread->mem_domain_info.mem_domain = NULL;
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_THREAD_STACK_INFO)
	thread->stack_info.start = (uintptr_t)pStack;
	thread->stack_info.size = (u32_t)stackSize;
#endif /* CONFIG_THREAD_STACK_INFO */
}

#endif /* !_ASMLANGUAGE */

#endif /* ZEPHYR_KERNEL_INCLUDE_KERNEL_ARCH_H_ */
