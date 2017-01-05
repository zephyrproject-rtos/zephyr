/* rtt_console.c - Console messages to a RAM buffer that is then read by
 * the Segger J-Link debugger
 */

/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
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


#include <kernel.h>
#include <misc/printk.h>
#include <device.h>
#include <init.h>
#include <rtt/SEGGER_RTT.h>

extern void __printk_hook_install(int (*fn)(int));
extern void __stdout_hook_install(int (*fn)(int));

static int rtt_console_out(int character)
{
	unsigned int key;
	char c = (char)character;

	key = irq_lock();
	SEGGER_RTT_WriteNoLock(0, &c, 1);
	irq_unlock(key);

	return character;
}

static int rtt_console_init(struct device *d)
{
	ARG_UNUSED(d);

	SEGGER_RTT_Init();

	__printk_hook_install(rtt_console_out);
	__stdout_hook_install(rtt_console_out);

	return 0;
}

SYS_INIT(rtt_console_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
