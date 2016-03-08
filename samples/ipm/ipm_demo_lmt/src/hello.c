/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
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

#include <misc/printk.h>
#include <zephyr.h>
#include <ipm.h>
#include <ipm/ipm_quark_se.h>

QUARK_SE_IPM_DEFINE(ping_ipm, 0, QUARK_SE_IPM_OUTBOUND);
QUARK_SE_IPM_DEFINE(message_ipm0, 1, QUARK_SE_IPM_OUTBOUND);
QUARK_SE_IPM_DEFINE(message_ipm1, 2, QUARK_SE_IPM_OUTBOUND);
QUARK_SE_IPM_DEFINE(message_ipm2, 3, QUARK_SE_IPM_OUTBOUND);

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  1000
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 1000)
#define SCSS_REGISTER_BASE		0xB0800000
#define SCSS_SS_STS                    0x0604

#define PING_TICKS 100
#define STACKSIZE 2000

#define MSG_FIBER_PRI		6
#define MAIN_FIBER_PRI		2
#define PING_FIBER_PRI		4

char fiber_stacks[2][STACKSIZE];

uint32_t scss_reg(uint32_t offset)
{
	volatile uint32_t *ret = (volatile uint32_t *)(SCSS_REGISTER_BASE +
						       offset);
	return *ret;
}

static const char dat1[] = "abcdefghijklmno";
static const char dat2[] = "pqrstuvwxyz0123";


void message_source(struct device *ipm)
{
	uint8_t counter = 0;

	printk("sending messages for IPM device %x\n", ipm);

	while (1) {
		ipm_send(ipm, 1, counter++, dat1, 16);
		ipm_send(ipm, 1, counter++, dat2, 16);
	}
}


void message_source_task_0(void)
{
	message_source(device_get_binding("message_ipm0"));
}

void message_source_task_1(void)
{
	message_source(device_get_binding("message_ipm1"));
}

void message_source_task_2(void)
{
	message_source(device_get_binding("message_ipm2"));
}

void ping_source_fiber(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	struct device *ipm = device_get_binding("ping_ipm");

	while (1) {
		fiber_sleep(PING_TICKS);
		printk("pinging arc for counter status\n");
		ipm_send(ipm, 1, 0, NULL, 0);
	}
}

void main_fiber(int arg1, int arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	int ctr = 0;
	uint32_t ss_sts;

	while (1) {
		/* say "hello" */
		printk("Hello from lakemont! (%d) ", ctr++);

		ss_sts = scss_reg(SCSS_SS_STS);
		switch (ss_sts) {
		case 0x4000:
			printk("ARC is halted");
			break;
		case 0x0400:
			printk("ARC is sleeping");
			break;
		case 0:
			printk("ARC is running");
			break;
		default:
			printk("ARC status: %x", ss_sts);
			break;
		}

		printk(", mailbox status: %x mask %x\n", scss_reg(0xac0),
		       scss_reg(0x4a0));

		/* wait a while, then let other task have a turn */
		fiber_sleep(SLEEPTICKS);
	}
}

void main_task(void)
{
	printk("===== app started ========\n");

	task_fiber_start(&fiber_stacks[0][0], STACKSIZE, main_fiber,
			 0, 0, MAIN_FIBER_PRI, 0);

	task_fiber_start(&fiber_stacks[1][0], STACKSIZE, ping_source_fiber,
			 0, 0, PING_FIBER_PRI, 0);

}

