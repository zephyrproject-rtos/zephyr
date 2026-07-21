/*
 * Copyright (c) 2023 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(chan_a, int, NULL, NULL, ZBUS_OBSERVERS(l1, ms1, ms2, s1, l2), 0);

static void t1_thread(void *ptr1, void *ptr2, void *ptr3);
K_THREAD_DEFINE(t1_id, CONFIG_MAIN_STACK_SIZE, t1_thread, NULL, NULL, NULL, 5, 0, 0);

ZBUS_SUBSCRIBER_DEFINE(s1, 4);
static void s1_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	int err;
	int a = 0;
	const struct zbus_channel *chan;

	IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (zbus_obs_attach_to_thread(&s1);));

	while (1) {
		err = zbus_sub_wait(&s1, &chan, K_FOREVER);
		if (err) {
			return;
		}

		/* Faking some workload */
		k_busy_wait(200000);

		LOG_INF("N -> S1:  T1 prio %d", k_thread_priority_get(t1_id));

		err = zbus_chan_read(chan, &a, K_FOREVER);
		if (err) {
			return;
		}
		LOG_INF("%d -> S1:  T1 prio %d", a, k_thread_priority_get(t1_id));
	}
}
K_THREAD_DEFINE(s1_id, CONFIG_MAIN_STACK_SIZE, s1_thread, NULL, NULL, NULL, 2, 0, 0);

ZBUS_MSG_SUBSCRIBER_DEFINE(ms1);
static void ms1_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	int err;
	const struct zbus_channel *chan;
	int a = 0;

	IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (zbus_obs_attach_to_thread(&ms1);));

	while (1) {
		err = zbus_sub_wait_msg(&ms1, &chan, &a, K_FOREVER);
		if (err) {
			return;
		}

		/* Faking some workload */
		k_busy_wait(200000);

		LOG_INF("%d -> MS1:  T1 prio %d", a, k_thread_priority_get(t1_id));
	}
}
K_THREAD_DEFINE(ms1_id, CONFIG_MAIN_STACK_SIZE, ms1_thread, NULL, NULL, NULL, 3, 0, 0);

ZBUS_MSG_SUBSCRIBER_DEFINE(ms2);
static void ms2_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	int err;
	const struct zbus_channel *chan;
	int a = 0;

	IF_ENABLED(CONFIG_ZBUS_PRIORITY_BOOST, (zbus_obs_attach_to_thread(&ms2);));

	while (1) {
		err = zbus_sub_wait_msg(&ms2, &chan, &a, K_FOREVER);
		if (err) {
			return;
		}

		/* Faking some workload */
		k_busy_wait(200 * USEC_PER_MSEC);

		LOG_INF("%d -> MS2:  T1 prio %d", a, k_thread_priority_get(t1_id));
	}
}
K_THREAD_DEFINE(ms2_id, CONFIG_MAIN_STACK_SIZE, ms2_thread, NULL, NULL, NULL, 4, 0, 0);

static void l1_callback(const struct zbus_channel *chan)
{
	LOG_INF("%d ---> L1: T1 prio %d", *((int *)zbus_chan_const_msg(chan)),
		k_thread_priority_get(t1_id));
}
ZBUS_LISTENER_DEFINE(l1, l1_callback);

static void l2_callback(const struct zbus_channel *chan)
{
	LOG_INF("%d ---> L2: T1 prio %d", *((int *)zbus_chan_const_msg(chan)),
		k_thread_priority_get(t1_id));
}
ZBUS_LISTENER_DEFINE(l2, l2_callback);

static void t1_thread(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	int err;
	int a = 0;

	while (1) {
		LOG_INF("--------------");

		if (a == 2) {
			zbus_obs_set_enable(&s1, false);
		} else if (a == 4) {
			zbus_obs_set_enable(&ms1, false);
		} else if (a == 6) {
			zbus_obs_set_enable(&s1, true);
			zbus_obs_set_enable(&ms1, true);
		}

		LOG_INF("%d -> T1: prio before %d", a, k_thread_priority_get(k_current_get()));
		err = zbus_chan_pub(&chan_a, &a, K_FOREVER);
		if (err) {
			return;
		}
		LOG_INF("%d -> T1: prio after %d", a, k_thread_priority_get(k_current_get()));
		++a;

		k_msleep(2000);
	}
}
