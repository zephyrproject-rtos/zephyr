/*
 * Copyright (c) 2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_runtime.h>
#include <syscall_handler.h>
/**
 * @brief Default character output routine that does nothing
 * @param c Character to swallow
 *
 * @return 0
 */
static int _nop_char_out(int c)
{
	ARG_UNUSED(c);

	/* do nothing */
	return 0;
}

int (*_char_out)(int) = _nop_char_out;

void __printk_hook_install(int (*fn)(int))
{
	_char_out = fn;
}

void *__printk_get_hook(void)
{
	return _char_out;
}

void _impl__k_str_out(char *c, size_t n)
{
	int i;

	for (i = 0; i < n; i++) {
		_k_char_out(c[i]);
	}
}

#ifdef CONFIG_USERSPACE
u32_t _handler__k_str_out(u32_t c, u32_t n, u32_t arg3, u32_t arg4,
			  u32_t arg5, u32_t arg6, void *ssf)
{
	_SYSCALL_ARG2;

	_SYSCALL_MEMORY(c, n, 0, ssf);
	_impl__k_str_out((char *)c, n);

	return 0;
}
#endif
