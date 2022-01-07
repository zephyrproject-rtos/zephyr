/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/printk.h>
#include <device.h>
#include <init.h>
#include <soc.h>

extern void __stdout_hook_install(int (*hook)(int));
extern void __printk_hook_install(int (*fn)(int));

static int esp32_hw_console_out(int character)
{
	/* console over cdc on esp32 uses the same uart rom
	 * functions
	 */
	esp_rom_usb_uart_tx_one_char(character);
	return 0;
}

static int esp32_hw_console_init(const struct device *d)
{
	ARG_UNUSED(d);

	__stdout_hook_install(esp32_hw_console_out);
	__printk_hook_install(esp32_hw_console_out);

	return 0;
}

SYS_INIT(esp32_hw_console_init, PRE_KERNEL_1, CONFIG_CONSOLE_INIT_PRIORITY);
