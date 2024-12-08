/*
 * Copyright (c) 2023 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "messages.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

#define STACK_SIZE (CONFIG_MAIN_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE)

static struct k_thread pub_thread;
static K_THREAD_STACK_DEFINE(pub_thread_sz, STACK_SIZE);
static struct k_thread s1_thread;
static K_THREAD_STACK_DEFINE(s1_thread_sz, STACK_SIZE);
static struct k_thread ms1_thread;
static K_THREAD_STACK_DEFINE(ms1_thread_sz, STACK_SIZE);

struct msg_testing_01 {
	int seq;
	bool must_detach;
};

ZBUS_CHAN_DEFINE(chan_testing_01,       /* Name */
		 struct msg_testing_01, /* Message type */

		 NULL,                              /* Validator */
		 NULL,                              /* User data */
		 ZBUS_OBSERVERS(lis1, sub1, msub1), /* observers */
		 ZBUS_MSG_INIT(0)                   /* Initial value major 0, minor 1, build 1023 */
);

static void consumer_sub_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr3);

	zbus_obs_attach_to_thread(ptr1);

	char *name = ptr2;
	const struct zbus_observer *sub = ptr1;
	const struct zbus_channel *chan;
	struct msg_testing_01 msg;

	while (1) {
		if (zbus_sub_wait(sub, &chan, K_FOREVER) != 0) {
			k_oops();
		}
		zbus_chan_read(chan, &msg, K_FOREVER);

		printk("%s level: %d\n", name, msg.seq);

		if (msg.must_detach) {
			zbus_obs_detach_from_thread(sub);
		}
	}
}

ZBUS_SUBSCRIBER_DEFINE(sub1, 4);

static void consumer_msg_sub_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr3);

	zbus_obs_attach_to_thread(ptr1);

	char *name = ptr2;
	const struct zbus_observer *msub = ptr1;
	const struct zbus_channel *chan;
	struct msg_testing_01 msg;

	while (1) {
		if (zbus_sub_wait_msg(msub, &chan, &msg, K_FOREVER) != 0) {
			k_oops();
		}
		printk("%s level: %d\n", name, msg.seq);

		if (msg.must_detach) {
			zbus_obs_detach_from_thread(msub);
		}
	}
}

ZBUS_MSG_SUBSCRIBER_DEFINE(msub1);

static K_SEM_DEFINE(sync_sem, 1, 1);
static K_SEM_DEFINE(done_sem, 0, 1);

static struct msg_testing_01 msg = {.seq = 0};

static void publisher_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	while (1) {
		k_sem_take(&sync_sem, K_FOREVER);
		zbus_chan_pub(&chan_testing_01, &msg, K_FOREVER);
		k_msleep(100);
		k_sem_give(&done_sem);
	}
}

static inline void _pub_and_sync(void)
{
	k_sem_give(&sync_sem);
	k_sem_take(&done_sem, K_FOREVER);
}

static k_tid_t pub_thread_id;

static int prio;

static void listener_callback(const struct zbus_channel *chan)
{
	prio = k_thread_priority_get(pub_thread_id);
}

ZBUS_LISTENER_DEFINE(lis1, listener_callback);

ZTEST(hlp_priority_boost, test_priority_elevation)
{
	pub_thread_id = k_thread_create(&pub_thread, pub_thread_sz, STACK_SIZE, publisher_thread,
					NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	(void)k_thread_create(&s1_thread, s1_thread_sz, STACK_SIZE, consumer_sub_thread,
			      (void *)&sub1, "sub1", NULL, K_PRIO_PREEMPT(3), 0, K_NO_WAIT);
	(void)k_thread_create(&ms1_thread, ms1_thread_sz, STACK_SIZE, consumer_msg_sub_thread,
			      (void *)&msub1, "msub1", NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);

	_pub_and_sync();
	zassert_true(prio == 1, "The priority must be 1, but it is %d", prio);

	++msg.seq;

	zbus_obs_set_enable(&msub1, false);
	_pub_and_sync();
	zassert_true(prio == 2, "The priority must be 2, but it is %d", prio);
	zbus_obs_set_enable(&msub1, true);

	++msg.seq;

	_pub_and_sync();
	zassert_true(prio == 1, "The priority must be 1, but it is %d", prio);

	++msg.seq;

	zbus_obs_set_chan_notification_mask(&msub1, &chan_testing_01, true);
	bool is_masked;

	zbus_obs_is_chan_notification_masked(&msub1, &chan_testing_01, &is_masked);
	zassert_true(is_masked, NULL);
	_pub_and_sync();
	zassert_true(prio == 2, "The priority must be 2, but it is %d", prio);
	zbus_obs_set_chan_notification_mask(&msub1, &chan_testing_01, false);

	++msg.seq;

	zbus_obs_set_enable(&msub1, false);
	zbus_obs_set_enable(&sub1, false);
	_pub_and_sync();
	zassert_true(prio == 8, "The priority must be 8, but it is %d", prio);
	zbus_obs_set_chan_notification_mask(&msub1, &chan_testing_01, false);
	zbus_obs_set_enable(&msub1, true);
	zbus_obs_set_enable(&sub1, true);

	++msg.seq;
	msg.must_detach = true;
	_pub_and_sync();
	zassert_true(prio == 1, "The priority must be 1, but it is %d", prio);
	++msg.seq;
	/* Checking if the detach command took effect on both observers */
	_pub_and_sync();
	zassert_true(prio == 8, "The priority must be 8, but it is %d", prio);
}

ZTEST_SUITE(hlp_priority_boost, NULL, NULL, NULL, NULL, NULL);
