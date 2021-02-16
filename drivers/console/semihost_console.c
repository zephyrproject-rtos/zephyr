/*
 * Copyright (c) 2019 LuoZhongYao
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <kernel_arch_interface.h>

extern void __stdout_hook_install(int (*fn)(int));

#define SYS_WRITEC 0x03

int arch_printk_char_out(int _c)
{
	char c = _c;

#if defined(CONFIG_CPU_CORTEX_M)

	register unsigned long r0 __asm__("r0") = SYS_WRITEC;
	register void *r1 __asm__("r1") = &c;

	__asm__ __volatile__ ("bkpt 0xab" : : "r" (r0), "r" (r1) : "memory");

#elif defined(CONFIG_ARM64)

	register unsigned long x0 __asm__("x0") = SYS_WRITEC;
	register void *x1 __asm__("x1") = &c;

	__asm__ volatile ("hlt 0xf000" : : "r" (x0), "r" (x1) : "memory");

#else
#error "unsupported CPU type"
#endif

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

SYS_INIT(semihost_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
