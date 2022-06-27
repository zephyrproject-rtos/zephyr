/* ipm_console_send.c - Console messages to another processor */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/drivers/console/ipm_console.h>

static const struct device *ipm_console_device;

static int consoleOut(int character)
{
	if (character == '\r') {
		return character;
	}

	/*
	 * We just stash the character into the id field and don't supply
	 * any extra data
	 */
	ipm_send(ipm_console_device, 1, character, NULL, 0);

	return character;
}

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

int ipm_console_sender_init(const struct device *d)
{
	const struct ipm_console_sender_config_info *config_info;

	config_info = d->config;
	ipm_console_device = device_get_binding(config_info->bind_to);

	if (!ipm_console_device) {
		printk("unable to bind IPM console sender to '%s'\n",
		       config_info->bind_to);
		return -EINVAL;
	}

	if (config_info->flags & IPM_CONSOLE_STDOUT) {
		__stdout_hook_install(consoleOut);
	}
	if (config_info->flags & IPM_CONSOLE_PRINTK) {
		__printk_hook_install(consoleOut);
	}

	return 0;
}
