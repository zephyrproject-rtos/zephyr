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
	static unsigned short n;
	static unsigned char buf[CONFIG_SEMIHOST_CONSOLE_OUTPUT_BUFFER_SIZE];

	buf[n++] = ch;
	buf[n] = 0;

	if (ch == 0 || n >= sizeof(buf) - 2 || ch == '\r' || ch == '\n') {
		__asm__ __volatile__ (
			"movs	r1, %0\n"
			"movs	r0, #4\n"
			"bkpt	0xab\n"
			:
			: "r" (buf)
			: "r0", "r1");
		n = 0;
	}

	return ch;
}

static int semihost_console_init(struct device *dev)
{
	ARG_UNUSED(dev);

	__printk_hook_install(semihost_console_out);
	__stdout_hook_install(semihost_console_out);

	return 0;
}

SYS_INIT(semihost_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
