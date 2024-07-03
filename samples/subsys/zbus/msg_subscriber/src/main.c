/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_REGISTER(sample, CONFIG_LOG_MAX_LEVEL);

extern struct sys_heap _system_heap;
static size_t total_allocated;

void on_heap_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	total_allocated += bytes;
	LOG_INF(" AL Memory allocated %u bytes. Total allocated %u bytes", (unsigned int)bytes,
		(unsigned int)total_allocated);
}

void on_heap_free(uintptr_t heap_id, void *mem, size_t bytes)
{
	total_allocated -= bytes;
	LOG_INF(" FR Memory freed %u bytes. Total allocated %u bytes", (unsigned int)bytes,
		(unsigned int)total_allocated);
}

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC)

HEAP_LISTENER_ALLOC_DEFINE(my_heap_listener_alloc, HEAP_ID_FROM_POINTER(&_system_heap),
			   on_heap_alloc);

HEAP_LISTENER_FREE_DEFINE(my_heap_listener_free, HEAP_ID_FROM_POINTER(&_system_heap), on_heap_free);

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC */
struct acc_msg {
	int x;
	int y;
	int z;
};

ZBUS_CHAN_DEFINE(acc_data_chan,  /* Name */
		 struct acc_msg, /* Message type */

		 NULL, /* Validator */
		 NULL, /* User data */
		 ZBUS_OBSERVERS(bar_sub1, bar_msg_sub1, bar_msg_sub2, bar_msg_sub3, bar_msg_sub4,
				bar_msg_sub5, bar_msg_sub6, bar_msg_sub7, bar_msg_sub8,
				bar_msg_sub9, foo_lis), /* observers */
		 ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0)  /* Initial value */
);

static void listener_callback_example(const struct zbus_channel *chan)
{
	const struct acc_msg *acc = zbus_chan_const_msg(chan);

	LOG_INF("From listener foo_lis -> Acc x=%d, y=%d, z=%d", acc->x, acc->y, acc->z);
}

ZBUS_LISTENER_DEFINE(foo_lis, listener_callback_example);

ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub1);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub2);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub3);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub4);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub5);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub6);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub7);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub8);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub9);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub10);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub11);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub12);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub13);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub14);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub15);
ZBUS_MSG_SUBSCRIBER_DEFINE(bar_msg_sub16);

ZBUS_SUBSCRIBER_DEFINE(bar_sub1, 4);
ZBUS_SUBSCRIBER_DEFINE(bar_sub2, 4);

static void msg_subscriber_task(void *sub)
{
	const struct zbus_channel *chan;

	struct acc_msg acc;

	const struct zbus_observer *subscriber = sub;

	while (!zbus_sub_wait_msg(subscriber, &chan, &acc, K_FOREVER)) {
		if (&acc_data_chan != chan) {
			LOG_ERR("Wrong channel %p!", chan);

			continue;
		}
		LOG_INF("From msg subscriber %s -> Acc x=%d, y=%d, z=%d", zbus_obs_name(subscriber),
			acc.x, acc.y, acc.z);
	}
}

K_THREAD_DEFINE(subscriber_task_id1, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub1,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id2, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub2,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id3, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub3,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id4, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub4,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id5, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub5,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id6, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub6,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id7, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub7,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id8, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub8,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id9, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub9,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id10, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub10,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id11, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub11,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id12, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub12,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id13, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub13,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id14, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub14,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id15, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub15,
		NULL, NULL, 3, 0, 0);
K_THREAD_DEFINE(subscriber_task_id16, CONFIG_MAIN_STACK_SIZE, msg_subscriber_task, &bar_msg_sub16,
		NULL, NULL, 3, 0, 0);

static void subscriber_task(void *sub)
{
	const struct zbus_channel *chan;

	struct acc_msg acc;

	const struct zbus_observer *subscriber = sub;

	while (!zbus_sub_wait(subscriber, &chan, K_FOREVER)) {
		if (&acc_data_chan != chan) {
			LOG_ERR("Wrong channel %p!", chan);

			continue;
		}
		zbus_chan_read(chan, &acc, K_MSEC(250));

		LOG_INF("From subscriber %s -> Acc x=%d, y=%d, z=%d", zbus_obs_name(subscriber),
			acc.x, acc.y, acc.z);
	}
}

K_THREAD_DEFINE(subscriber_task_id17, CONFIG_MAIN_STACK_SIZE, subscriber_task, &bar_sub1, NULL,
		NULL, 2, 0, 0);
K_THREAD_DEFINE(subscriber_task_id18, CONFIG_MAIN_STACK_SIZE, subscriber_task, &bar_sub2, NULL,
		NULL, 4, 0, 0);

ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_sub2, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub10, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub11, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub12, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub13, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub14, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub15, 3);
ZBUS_CHAN_ADD_OBS(acc_data_chan, bar_msg_sub16, 3);

static struct acc_msg acc = {.x = 1, .y = 10, .z = 100};

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION)
#include <zephyr/net/buf.h>

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC)
NET_BUF_POOL_HEAP_DEFINE(isolated_pool, (CONFIG_ZBUS_MSG_SUBSCRIBER_SAMPLE_ISOLATED_BUF_POOL_SIZE),
			 (sizeof(struct zbus_channel *)), NULL);
#else
NET_BUF_POOL_FIXED_DEFINE(isolated_pool, (CONFIG_ZBUS_MSG_SUBSCRIBER_SAMPLE_ISOLATED_BUF_POOL_SIZE),
			  (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE),
			  sizeof(struct zbus_channel *), NULL);
#endif
#endif

int main(void)
{

	total_allocated = 0;
#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_ISOLATION)
	zbus_chan_set_msg_sub_pool(&acc_data_chan, &isolated_pool);
#endif

#if defined(CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC)

	heap_listener_register(&my_heap_listener_alloc);
	heap_listener_register(&my_heap_listener_free);

#endif /* CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC */

	while (1) {
		LOG_INF("----> Publishing to %s channel", zbus_chan_name(&acc_data_chan));
		zbus_chan_pub(&acc_data_chan, &acc, K_NO_WAIT);
		acc.x += 1;
		acc.y += 10;
		acc.z += 100;
		k_msleep(1000);
	}

	return 0;
}
