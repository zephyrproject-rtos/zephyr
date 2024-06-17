/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#define CONSOLE_OUT_ADDR (DT_REG_ADDR(DT_CHOSEN(zephyr_console)))

int arch_printk_char_out(int c)
{
	sys_write32((uint32_t)c, CONSOLE_OUT_ADDR);

	/* Make sure register write is done before proceeding. */
	__asm__ volatile("fence iorw,iorw");

	return 0;
}

#if defined(CONFIG_STDOUT_CONSOLE)
extern void __stdout_hook_install(int (*hook)(int));
#else
#define __stdout_hook_install(x)                                                                   \
	do { /* nothing */                                                                         \
	} while ((0))
#endif

#if defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int));
#else
#define __printk_hook_install(x)                                                                   \
	do { /* nothing */                                                                         \
	} while ((0))
#endif

static void whisper_console_hook_install(void)
{
	__stdout_hook_install(arch_printk_char_out);
	__printk_hook_install(arch_printk_char_out);
}

static int whisper_console_init(void)
{
	whisper_console_hook_install();

	return 0;
}

SYS_INIT(whisper_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
