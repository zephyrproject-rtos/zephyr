/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_structs.h>
#include <logging/kernel_event_logger.h>
#include <kernel_structs.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <zephyr.h>

K_THREAD_STACK_DEFINE(threadA_stack, 1024);
K_THREAD_STACK_DEFINE(threadB_stack, 1024);
K_THREAD_STACK_DEFINE(event_logger_stack, 4096);

K_SEM_DEFINE(sem_sync, 0, 1);	/* starts off "not available" */

static struct k_thread threadA_data;
static struct k_thread threadB_data;
static u32_t timestamp;
char event_type[][12] = {"REDAY_Q", "PEND_STATE", "EXIT_STATE"};

static void event_logger(void)
{
	u32_t event_data[4];
	u16_t event_id;
	u8_t dropped;
	u8_t event_data_size = (u8_t)ARRAY_SIZE(event_data);
	int ret;

	for (;;) {

		ret = sys_k_event_logger_get_wait(&event_id,
						  &dropped,
						  event_data,
						  &event_data_size);
		if (ret < 0) {
			continue;
		}

		timestamp = event_data[0];

		switch (event_id) {
		case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
			printk("tid of context switched thread = %x at "
			       "time = %d\n", event_data[1], timestamp);
			break;
		case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
			break;
		case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
			break;
		case KERNEL_EVENT_LOGGER_THREAD_EVENT_ID:
			printk("thread = %x, is moved to = %s ,at time = %d\n"
			, event_data[1], event_type[event_data[2]], timestamp);
			break;
		default:
			break;
		}

	}
}

static void threadB(void)
{
	unsigned int i = 10;

	for (; i ; i--) {
		k_sleep(MSEC_PER_SEC / 2);
	}

	k_sem_take(&sem_sync, K_FOREVER);
}

static void threadA(void)
{
	unsigned int j = 10;

	for (; j ; j--) {

		k_sleep(MSEC_PER_SEC);
	}

	k_sem_give(&sem_sync);
}

void main(void)
{
	k_thread_create(&threadB_data, threadB_stack,
			K_THREAD_STACK_SIZEOF(threadB_stack),
			(k_thread_entry_t)threadB,
			NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, 0);

	k_thread_create(&threadA_data, threadA_stack,
			K_THREAD_STACK_SIZEOF(threadA_stack),
			(k_thread_entry_t)threadA,
			NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, 0);

	event_logger();
}
