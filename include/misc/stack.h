/**
 * @file stack.h
 * Stack usage analysis helpers
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MISC_STACK_H_
#define _MISC_STACK_H_

#include <misc/printk.h>

#if defined(CONFIG_INIT_STACKS)
static inline size_t stack_unused_space_get(const char *stack, size_t size)
{
	size_t unused = 0;
	int i;

	/* TODO Currently all supported platforms have stack growth down and
	 * there is no Kconfig option to configure it so this always build
	 * "else" branch.  When support for platform with stack direction up
	 * (or configurable direction) is added this check should be confirmed
	 * that correct Kconfig option is used.
	 */
#if defined(CONFIG_STACK_GROWS_UP)
	for (i = size - 1; i >= 0; i--) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#else
	for (i = 0; i < size; i++) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#endif
	return unused;
}
#else
static inline size_t stack_unused_space_get(const char *stack, size_t size)
{
	return 0;
}
#endif

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
static inline void stack_analyze(const char *name, const char *stack,
				 unsigned int size)
{
	unsigned int pcnt, unused = 0;

	unused = stack_unused_space_get(stack, size);

	/* Calculate the real size reserved for the stack */
	pcnt = ((size - unused) * 100) / size;

	printk("%s (real size %u):\tunused %u\tusage %u / %u (%u %%)\n", name,
	       size, unused, size - unused, size, pcnt);
}
#else
static inline void stack_analyze(const char *name, const char *stack,
				 unsigned int size)
{
}
#endif

#define STACK_ANALYZE(name, sym) \
	stack_analyze(name, K_THREAD_STACK_BUFFER(sym), \
		      K_THREAD_STACK_SIZEOF(sym))

#endif /* _MISC_STACK_H_ */
