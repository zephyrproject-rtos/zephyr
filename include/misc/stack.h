/**
 * @file stack.h
 * Stack usage analysis helpers
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MISC_STACK_H_
#define ZEPHYR_INCLUDE_MISC_STACK_H_

#include <misc/printk.h>

#if defined(CONFIG_INIT_STACKS)
static inline size_t stack_unused_space_get(const char *stack, size_t size)
{
	size_t unused = 0;

#ifdef CONFIG_STACK_SENTINEL
	/* First 4 bytes of the stack buffer reserved for the sentinel
	 * value, it won't be 0xAAAAAAAA for thread stacks.
	 */
	const unsigned char *checked_stack = (const unsigned char *)stack + 4;
#else
	const unsigned char *checked_stack = (const unsigned char *)stack;
#endif

	/* TODO Currently all supported platforms have stack growth down and
	 * there is no Kconfig option to configure it so this always build
	 * "else" branch.  When support for platform with stack direction up
	 * (or configurable direction) is added this check should be confirmed
	 * that correct Kconfig option is used.
	 */
#if defined(CONFIG_STACK_GROWS_UP)
	int i;
	for (i = size - 1; i >= 0; i--) {
		if (checked_stack[i] == 0xaaU) {
			unused++;
		} else {
			break;
		}
	}
#else
	size_t i;
	for (i = 0; i < size; i++) {
		if (checked_stack[i] == 0xaaU) {
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
/**
 * @brief Analyze stacks
 *
 * Use this macro to get information about stack usage
 *
 * @param name Name of the stack
 * @param sym The symbol of the stack
 */
#define STACK_ANALYZE(name, sym) \
	stack_analyze(name, K_THREAD_STACK_BUFFER(sym), \
		      K_THREAD_STACK_SIZEOF(sym))

#endif /* ZEPHYR_INCLUDE_MISC_STACK_H_ */
