/* ipm_console_send.c - Console messages to another processor */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <kernel.h>
#include <misc/printk.h>
#include <ipm.h>
#include <console/ipm_console.h>

static struct device *ipm_console_device;

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

int ipm_console_sender_init(struct device *d)
{
	const struct ipm_console_sender_config_info *config_info;

	config_info = d->config->config_info;
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
