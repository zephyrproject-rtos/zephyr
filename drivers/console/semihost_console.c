/*
 * Copyright (c) 2019 LuoZhongYao
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/arch/common/semihost.h>

extern void __stdout_hook_install(int (*fn)(int));

int arch_printk_char_out(int _c)
{
	semihost_poll_out((char)_c);
	return 0;
}

static int semihost_console_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/*
	 * The printk output callback is arch_printk_char_out by default and
	 * is installed at link time. That makes printk() usable very early.
	 *
	 * We still need to install the stdout callback manually at run time.
	 */
	__stdout_hook_install(arch_printk_char_out);

	return 0;
}

SYS_INIT(semihost_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
