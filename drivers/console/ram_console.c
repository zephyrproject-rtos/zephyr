/* ram_console.c - Console messages to a RAM buffer */

/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/devicetree_regions.h>

#ifdef CONFIG_RAM_CONSOLE_BUFFER_SECTION
#if !DT_HAS_CHOSEN(zephyr_ram_console)
#error "Lack of chosen property zephyr,ram_console!"
#elif (CONFIG_RAM_CONSOLE_BUFFER_SIZE > DT_REG_SIZE(DT_CHOSEN(zephyr_ram_console)))
#error "Custom RAM console buffer exceeds the section size!"
#endif

#define RAM_CONSOLE_BUF_ATTR	\
	__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(DT_CHOSEN(zephyr_ram_console)))))
#else
#define RAM_CONSOLE_BUF_ATTR
#endif

char ram_console_buf[CONFIG_RAM_CONSOLE_BUFFER_SIZE] RAM_CONSOLE_BUF_ATTR;
char *ram_console;
static int pos;

static int ram_console_out(int character)
{
	ram_console[pos] = (char)character;
	/* Leave one byte to ensure we're always NULL-terminated */
	pos = (pos + 1) % (CONFIG_RAM_CONSOLE_BUFFER_SIZE - 1);
	return character;
}

static int ram_console_init(void)
{
#ifdef CONFIG_RAM_CONSOLE_BUFFER_SECTION
	mm_reg_t ram_console_va;

	device_map((mm_reg_t *)&ram_console_va, DT_REG_ADDR(DT_CHOSEN(zephyr_ram_console)),
		   CONFIG_RAM_CONSOLE_BUFFER_SIZE, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	ram_console = (char *)ram_console_va,
#else
	ram_console = ram_console_buf,
#endif
	__printk_hook_install(ram_console_out);
	__stdout_hook_install(ram_console_out);

	return 0;
}

SYS_INIT(ram_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
