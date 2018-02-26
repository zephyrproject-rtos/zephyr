/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <zephyr.h>
#include <logging/kernel_event_logger.h>
#include <misc/printk.h>
#include <misc/util.h>

K_THREAD_STACK_DEFINE(threadA_stack, 1024);
K_THREAD_STACK_DEFINE(threadB_stack, 1024);
K_THREAD_STACK_DEFINE(event_logger_stack, 4096);

K_SEM_DEFINE(sem_sync, 0, 1);	/* starts off "not available" */

#define UNKNOWN_EVENT  0xaa

static struct k_thread threadA_data;
static struct k_thread threadB_data;
static u32_t timestamp;
static u8_t complete;
char event_type[][12] = {"READY_Q", "PEND_STATE", "EXIT_STATE"};

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
			complete = 1;
			break;
		case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
			complete = 1;
			break;
		case KERNEL_EVENT_LOGGER_THREAD_EVENT_ID:
			printk("thread = %x, is moved to = %s ,at time = %d\n"
			, event_data[1], event_type[event_data[2]], timestamp);
			break;
		default:
			event_id = UNKNOWN_EVENT;
			break;
		}

		zassert_not_equal(UNKNOWN_EVENT, event_id, "Unknown event");

		if (complete) {
			complete = 0;
			sys_k_event_logger_set_mask(0);
			break;
		}

	}
}

static void threadB(void)
{
	unsigned int i = 2;

	for (; i ; i--) {
		k_sleep(MSEC_PER_SEC / 2);
	}

	k_sem_take(&sem_sync, K_FOREVER);

	complete = 1;
}

static void threadA(void)
{
	unsigned int j = 2;

	for (; j ; j--) {

		k_sleep(MSEC_PER_SEC);
	}

	k_sem_give(&sem_sync);
}

void test_context_switch(void)
{
	u8_t event_id = KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID;

	sys_k_event_logger_set_mask(event_id);

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

void test_thread_event(void)
{
	u8_t event_id = KERNEL_EVENT_LOGGER_THREAD_EVENT_ID;

	sys_k_event_logger_set_mask(event_id);

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

void test_interrupt_event(void)
{
	u8_t event_id = KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID;

	sys_k_event_logger_set_mask(event_id);

	event_logger();
}

static unsigned int get_time(void)
{
	return k_uptime_get();
}

void test_coustom_time_stamp(void)
{
	u8_t event_id = KERNEL_EVENT_LOGGER_THREAD_EVENT_ID;

	sys_k_event_logger_set_mask(event_id);

	sys_k_event_logger_set_timer(get_time);

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

void test_sleep_event(void)
{
	u8_t event_id = KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID;

	sys_k_event_logger_set_mask(event_id);

	event_logger();
}

void test_main(void)
{
	k_thread_priority_set(k_current_get(), 3);

	ztest_test_suite(test_eventlogger,
			ztest_unit_test(test_sleep_event),
			ztest_unit_test(test_interrupt_event),
			ztest_unit_test(test_thread_event),
			ztest_unit_test(test_context_switch),
			ztest_unit_test(test_coustom_time_stamp));
	ztest_run_test_suite(test_eventlogger);
}
