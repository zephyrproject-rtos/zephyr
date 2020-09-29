/*
 * Copyright (c) 2019 LuoZhongYao
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/printk.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static int semihost_console_out(int ch)
{
	static unsigned char c;

	c = ch;
	__asm__ __volatile__ (
		"movs	r1, %0\n"
		"movs	r0, #3\n"
		"bkpt	0xab\n"
		:
		: "r" (&c)
		: "r0", "r1");
	return ch;
}

static int semihost_console_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	__printk_hook_install(semihost_console_out);
	__stdout_hook_install(semihost_console_out);

	return 0;
}

SYS_INIT(semihost_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
