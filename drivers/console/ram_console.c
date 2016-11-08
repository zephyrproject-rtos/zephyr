/* ram_console.c - Console messages to a RAM buffer */

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


#include <nanokernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

/* Extra byte to ensure we're always NULL-terminated */
char ram_console[CONFIG_RAM_CONSOLE_BUFFER_SIZE + 1];
static int pos;

static int ram_console_out(int character)
{
	ram_console[pos] = (char)character;
	pos = (pos + 1) % CONFIG_RAM_CONSOLE_BUFFER_SIZE;
	return character;
}

static int ram_console_init(struct device *d)
{
	ARG_UNUSED(d);
	__printk_hook_install(ram_console_out);
	__stdout_hook_install(ram_console_out);

	return 0;
}

SYS_INIT(ram_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
