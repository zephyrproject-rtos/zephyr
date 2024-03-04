/* ram_console.c - Console messages to a RAM buffer */

/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/printk-hooks.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/drivers/console/ram_console.h>

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

/* workaround the fact that device_map() is not defined for SoCs with no MMU */
#ifndef DEVICE_MMIO_IS_IN_RAM
#define device_map(virt, phys, size, flags) *(virt) = (phys)
#endif /* DEVICE_MMIO_IS_IN_RAM */

char ram_console_buf[CONFIG_RAM_CONSOLE_BUFFER_SIZE] RAM_CONSOLE_BUF_ATTR;
char *ram_console;
#ifdef CONFIG_RAM_CONSOLE_HEADER
static struct ram_console_header *header;
#else
static int pos;
#endif

static int ram_console_out(int character)
{
#ifdef CONFIG_RAM_CONSOLE_HEADER
	header->buf_addr[header->pos % header->buf_size] = (char)character;
	header->pos++;
	/* in case of overflow as it is absolute position */
	if (header->pos == 0)
		header->pos = (0xFFFFFFFF % header->buf_size) + 1;
#else
	ram_console[pos] = (char)character;
	/* Leave one byte to ensure we're always NULL-terminated */
	pos = (pos + 1) % (CONFIG_RAM_CONSOLE_BUFFER_SIZE - 1);
#endif

	return character;
}

static int ram_console_init(void)
{
#ifdef CONFIG_RAM_CONSOLE_BUFFER_SECTION
	mm_reg_t ram_console_va;

	device_map((mm_reg_t *)&ram_console_va, DT_REG_ADDR(DT_CHOSEN(zephyr_ram_console)),
		   CONFIG_RAM_CONSOLE_BUFFER_SIZE, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);
	ram_console = (char *)ram_console_va;
#else
	ram_console = ram_console_buf;
#endif
#ifdef CONFIG_RAM_CONSOLE_HEADER
	unsigned int size = CONFIG_RAM_CONSOLE_BUFFER_SIZE - 1;

	memset(ram_console, 0, size + 1);
	header = (struct ram_console_header *)ram_console;
	strcpy(header->flag_string, RAM_CONSOLE_HEAD_STR);
	header->buf_addr = (char *)(ram_console + RAM_CONSOLE_HEAD_SIZE);
	header->buf_size = size - RAM_CONSOLE_HEAD_SIZE;
	header->pos = 0;
#endif

	__printk_hook_install(ram_console_out);
	__stdout_hook_install(ram_console_out);

	return 0;
}

SYS_INIT(ram_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
