/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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

#define SLEEPTIME               1000

#define PING_TIME               1000
#define STACKSIZE               2000

#define MSG_FIBER_PRI           6
#define MAIN_FIBER_PRI          2
#define PING_FIBER_PRI          4
#define TASK_PRIO               7

K_THREAD_STACK_ARRAY_DEFINE(thread_stacks, 2, STACKSIZE);
static struct k_thread threads[2];

u32_t scss_reg(u32_t offset)
{
	volatile u32_t *ret = (volatile u32_t *)(SCSS_REGISTER_BASE +
						       offset);

	return *ret;
}

static const char dat1[] = "abcdefghijklmno";
static const char dat2[] = "pqrstuvwxyz0123";


void message_source(struct device *ipm)
{
	u8_t counter = 0;

	printk("sending messages for IPM device %p\n", ipm);

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

void ping_source_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct device *ipm = device_get_binding("ping_ipm");

	while (1) {
		k_sleep(PING_TIME);
		printk("pinging sensor subsystem (ARC) for counter status\n");
		ipm_send(ipm, 1, 0, NULL, 0);
	}
}

void main_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	int ctr = 0;
	u32_t ss_sts;

	while (1) {
		/* say "hello" */
		printk("Hello from application processor (x86)! (%d) ", ctr++);

		ss_sts = scss_reg(SCSS_SS_STS);
		switch (ss_sts) {
		case 0x4000:
			printk("Sensor Subsystem (ARC) is halted");
			break;
		case 0x0400:
			printk("Sensor Subsystem (ARC) is sleeping");
			break;
		case 0:
			printk("Sensor Subsystem (ARC) is running");
			break;
		default:
			printk("Sensor Subsystem (ARC) status: %x", ss_sts);
			break;
		}

		printk(", mailbox status: %x mask %x\n", scss_reg(0xac0),
		       scss_reg(0x4a0));

		/* wait a while, then let other task have a turn */
		k_sleep(SLEEPTIME);
	}
}

K_THREAD_DEFINE(MSG_TASK0, STACKSIZE, message_source_task_0, NULL, NULL, NULL,
		TASK_PRIO, 0, K_NO_WAIT);

K_THREAD_DEFINE(MSG_TASK1, STACKSIZE, message_source_task_1, NULL, NULL, NULL,
		TASK_PRIO, 0, K_NO_WAIT);

K_THREAD_DEFINE(MSG_TASK2, STACKSIZE, message_source_task_2, NULL, NULL, NULL,
		TASK_PRIO, 0, K_NO_WAIT);

void main(void)
{
	printk("===== app started ========\n");

	k_thread_create(&threads[0], &thread_stacks[0][0], STACKSIZE,
			main_thread, 0, 0, 0,
			K_PRIO_COOP(MAIN_FIBER_PRI), 0, 0);

	k_thread_create(&threads[1], &thread_stacks[1][0], STACKSIZE,
			ping_source_thread, 0, 0, 0,
			K_PRIO_COOP(PING_FIBER_PRI), 0, 0);
}

