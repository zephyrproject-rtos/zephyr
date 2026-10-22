/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "messages.h"

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DEFINE(version_chan,	     /* Name */
		 struct version_msg, /* Message type */

		 NULL,		       /* Validator */
		 NULL,		       /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 1023) /* Initial value major 0, minor 1, build 1023 */
);

ZBUS_CHAN_DEFINE(sensor_data_chan,  /* Name */
		 struct sensor_msg, /* Message type */

		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(fast_handler1_lis, fast_handler2_lis, fast_handler3_lis,
				delay_handler1_lis, delay_handler2_lis, delay_handler3_lis,
				thread_handler1_sub, thread_handler2_sub,
				thread_handler3_sub), /* observers */
		 ZBUS_MSG_INIT(0)		      /* Initial value {0} */
);

static void fh1_cb(const struct zbus_channel *chan)
{
	const struct sensor_msg *msg = zbus_chan_const_msg(chan);

	LOG_INF("Sensor msg processed by CALLBACK fh1: temp = %u, press = %u, humidity = %u",
		msg->temp, msg->press, msg->humidity);
}

ZBUS_LISTENER_DEFINE(fast_handler1_lis, fh1_cb);

static void fh2_cb(const struct zbus_channel *chan)
{
	const struct sensor_msg *msg = zbus_chan_const_msg(chan);

	LOG_INF("Sensor msg processed by CALLBACK fh2: temp = %u, press = %u, humidity = %u",
		msg->temp, msg->press, msg->humidity);
}

ZBUS_LISTENER_DEFINE(fast_handler2_lis, fh2_cb);

static void fh3_cb(const struct zbus_channel *chan)
{
	const struct sensor_msg *msg = zbus_chan_const_msg(chan);

	LOG_INF("Sensor msg processed by CALLBACK fh3: temp = %u, press = %u, humidity = %u",
		msg->temp, msg->press, msg->humidity);
}

ZBUS_LISTENER_DEFINE(fast_handler3_lis, fh3_cb);

struct sensor_wq_info {
	struct k_work work;
	const struct zbus_channel *chan;
	uint8_t handle;
};

static struct sensor_wq_info wq_handler1 = {.handle = 1};
static struct sensor_wq_info wq_handler2 = {.handle = 2};
static struct sensor_wq_info wq_handler3 = {.handle = 3};

static void wq_dh_cb(struct k_work *item)
{
	struct sensor_msg msg;
	struct sensor_wq_info *sens = CONTAINER_OF(item, struct sensor_wq_info, work);

	zbus_chan_read(sens->chan, &msg, K_MSEC(200));

	LOG_INF("Sensor msg processed by WORK QUEUE handler dh%u: temp = %u, press = %u, "
		"humidity = %u",
		sens->handle, msg.temp, msg.press, msg.humidity);
}

static void dh1_cb(const struct zbus_channel *chan)
{
	wq_handler1.chan = chan;

	k_work_submit(&wq_handler1.work);
}

ZBUS_LISTENER_DEFINE(delay_handler1_lis, dh1_cb);

static void dh2_cb(const struct zbus_channel *chan)
{
	wq_handler2.chan = chan;

	k_work_submit(&wq_handler2.work);
}

ZBUS_LISTENER_DEFINE(delay_handler2_lis, dh2_cb);

static void dh3_cb(const struct zbus_channel *chan)
{
	wq_handler3.chan = chan;

	k_work_submit(&wq_handler3.work);
}

ZBUS_LISTENER_DEFINE(delay_handler3_lis, dh3_cb);

int main(void)
{
	k_work_init(&wq_handler1.work, wq_dh_cb);
	k_work_init(&wq_handler2.work, wq_dh_cb);
	k_work_init(&wq_handler3.work, wq_dh_cb);

	struct version_msg *v = zbus_chan_msg(&version_chan);

	LOG_DBG("Sensor sample started, version %u.%u-%u!", v->major, v->minor, v->build);
	return 0;
}

ZBUS_SUBSCRIBER_DEFINE(thread_handler1_sub, 4);

static void thread_handler1_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&thread_handler1_sub, &chan, K_FOREVER)) {
		struct sensor_msg msg;

		zbus_chan_read(chan, &msg, K_MSEC(200));

		LOG_INF("Sensor msg processed by THREAD handler 1: temp = %u, press = %u, "
			"humidity = %u",
			msg.temp, msg.press, msg.humidity);
	}
}

K_THREAD_DEFINE(thread_handler1_id, 1024, thread_handler1_task, NULL, NULL, NULL, 3, 0, 0);

ZBUS_SUBSCRIBER_DEFINE(thread_handler2_sub, 4);

static void thread_handler2_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&thread_handler2_sub, &chan, K_FOREVER)) {
		struct sensor_msg msg;

		zbus_chan_read(chan, &msg, K_MSEC(200));

		LOG_INF("Sensor msg processed by THREAD handler 2: temp = %u, press = %u, "
			"humidity = %u",
			msg.temp, msg.press, msg.humidity);
	}
}

K_THREAD_DEFINE(thread_handler2_id, 1024, thread_handler2_task, NULL, NULL, NULL, 3, 0, 0);

ZBUS_SUBSCRIBER_DEFINE(thread_handler3_sub, 4);

static void thread_handler3_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&thread_handler3_sub, &chan, K_FOREVER)) {
		struct sensor_msg msg;

		zbus_chan_read(chan, &msg, K_MSEC(200));

		LOG_INF("Sensor msg processed by THREAD handler 3: temp = %u, press = %u, "
			"humidity = %u",
			msg.temp, msg.press, msg.humidity);
	}
}

K_THREAD_DEFINE(thread_handler3_id, 1024, thread_handler3_task, NULL, NULL, NULL, 3, 0, 0);
