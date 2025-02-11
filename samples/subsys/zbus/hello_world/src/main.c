/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct acc_msg {
	int x;
	int y;
	int z;
};

ZBUS_CHAN_DEFINE(version_chan,       /* Name */
		 struct version_msg, /* Message type */

		 NULL,                 /* Validator */
		 NULL,                 /* User data */
		 ZBUS_OBSERVERS_EMPTY, /* observers */
		 ZBUS_MSG_INIT(.major = 0, .minor = 1,
			       .build = 2) /* Initial value major 0, minor 1, build 2 */
);

ZBUS_CHAN_DEFINE(acc_data_chan,  /* Name */
		 struct acc_msg, /* Message type */

		 NULL,                                 /* Validator */
		 NULL,                                 /* User data */
		 ZBUS_OBSERVERS(foo_lis, bar_sub),     /* observers */
		 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0) /* Initial value */
);

static bool simple_chan_validator(const void *msg, size_t msg_size)
{
	ARG_UNUSED(msg_size);

	const int *simple = msg;

	if ((*simple >= 0) && (*simple < 10)) {
		return true;
	}

	return false;
}

ZBUS_CHAN_DEFINE(simple_chan, /* Name */
		 int,         /* Message type */

		 simple_chan_validator, /* Validator */
		 NULL,                  /* User data */
		 ZBUS_OBSERVERS_EMPTY,  /* observers */
		 0                      /* Initial value is 0 */
);

static void listener_callback_example(const struct zbus_channel *chan)
{
	const struct acc_msg *acc = zbus_chan_const_msg(chan);

	LOG_INF("From listener -> Acc x=%d, y=%d, z=%d", acc->x, acc->y, acc->z);
}

ZBUS_LISTENER_DEFINE(foo_lis, listener_callback_example);

ZBUS_SUBSCRIBER_DEFINE(bar_sub, 4);

static void subscriber_task(void)
{
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&bar_sub, &chan, K_FOREVER)) {
		struct acc_msg acc;

		if (&acc_data_chan == chan) {
			zbus_chan_read(&acc_data_chan, &acc, K_MSEC(500));

			LOG_INF("From subscriber -> Acc x=%d, y=%d, z=%d", acc.x, acc.y, acc.z);
		}
	}
}

K_THREAD_DEFINE(subscriber_task_id, CONFIG_MAIN_STACK_SIZE, subscriber_task, NULL, NULL, NULL, 3, 0,
		0);

static bool print_channel_data_iterator(const struct zbus_channel *chan, void *user_data)
{
	int *count = user_data;

	LOG_INF("%d - Channel %s:", *count, zbus_chan_name(chan));
	LOG_INF("      Message size: %d", zbus_chan_msg_size(chan));
	LOG_INF("      Observers:");

	++(*count);

	struct zbus_channel_observation *observation;

	for (int16_t i = chan->data->observers_start_idx, limit = chan->data->observers_end_idx;
	     i < limit; ++i) {
		STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

		__ASSERT(observation != NULL, "observation must be not NULL");

		LOG_INF("      - %s", observation->obs->name);
	}

	struct zbus_observer_node *obs_nd, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&chan->data->observers, obs_nd, tmp, node) {
		LOG_INF("      - %s", obs_nd->obs->name);
	}

	return true;
}

static bool print_observer_data_iterator(const struct zbus_observer *obs, void *user_data)
{
	int *count = user_data;

	LOG_INF("%d - %s %s", *count,
		obs->type == ZBUS_OBSERVER_LISTENER_TYPE ? "Listener" : "Subscriber",
		zbus_obs_name(obs));

	++(*count);

	return true;
}

int main(void)
{
	int err, value;
	struct acc_msg acc1 = {.x = 1, .y = 1, .z = 1};
	const struct version_msg *v = zbus_chan_const_msg(&version_chan);

	LOG_INF("Sensor sample started raw reading, version %u.%u-%u!", v->major, v->minor,
		v->build);

	int count = 0;

	LOG_INF("Channel list:");
	zbus_iterate_over_channels_with_user_data(print_channel_data_iterator, &count);

	count = 0;

	LOG_INF("Observers list:");
	zbus_iterate_over_observers_with_user_data(print_observer_data_iterator, &count);
	zbus_chan_pub(&acc_data_chan, &acc1, K_SECONDS(1));

	k_msleep(1000);

	acc1.x = 2;
	acc1.y = 2;
	acc1.z = 2;
	zbus_chan_pub(&acc_data_chan, &(acc1), K_SECONDS(1));

	value = 5;
	err = zbus_chan_pub(&simple_chan, &value, K_MSEC(200));

	if (err == 0) {
		LOG_INF("Pub a valid value to a channel with validator successfully.");
	}

	value = 15;
	err = zbus_chan_pub(&simple_chan, &value, K_MSEC(200));

	if (err == -ENOMSG) {
		LOG_INF("Pub an invalid value to a channel with validator successfully.");
	}
	return 0;
}
