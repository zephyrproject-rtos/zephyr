/**
 * @file stack.h
 * Stack usage analysis helpers
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_PRINTK)
#include <offsets.h>
#include <misc/printk.h>

static inline void stack_analyze(const char *name, const char *stack,
				 unsigned size)
{
	unsigned i, stack_offset, pcnt, unused = 0;

	/* The TCS is always placed on a 4-byte aligned boundary - if
	 * the stack beginning doesn't match that there will be some
	 * unused bytes in the beginning.
	 */
	stack_offset = K_THREAD_SIZEOF + ((4 - ((unsigned)stack % 4)) % 4);

/* TODO
 * Currently all supported platforms have stack growth down and there is no
 * Kconfig option to configure it so this always build "else" branch.
 * When support for platform with stack direction up (or configurable direction)
 * is added this check should be confirmed that correct Kconfig option is used.
 */
#if defined(CONFIG_STACK_GROWS_UP)
	for (i = size - 1; i >= stack_offset; i--) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#else
	for (i = stack_offset; i < size; i++) {
		if ((unsigned char)stack[i] == 0xaa) {
			unused++;
		} else {
			break;
		}
	}
#endif

	/* Calculate the real size reserved for the stack */
	size -= stack_offset;
	pcnt = ((size - unused) * 100) / size;

	printk("%s (real size %u):\tunused %u\tusage %u / %u (%u %%)\n", name,
	       size + stack_offset, unused, size - unused, size, pcnt);
}
#else
static inline void stack_analyze(const char *name, const char *stack,
				 unsigned size)
{
}
#endif
