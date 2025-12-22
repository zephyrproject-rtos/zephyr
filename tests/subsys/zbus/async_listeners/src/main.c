/*
 * Copyright (c) 2025 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <stdint.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct msg_ping {
	bool should_pong;
};

ZBUS_CHAN_DEFINE(chan_ping, struct msg_ping, NULL, NULL, ZBUS_OBSERVERS(alis1, alis2),
		 ZBUS_MSG_INIT(.should_pong = false));

K_MSGQ_DEFINE(msgq_pong, sizeof(uint32_t),
	      CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE +
		      2 /* only 2 should_pong==true sent before bursting */,
	      4);
K_SEM_DEFINE(sem_alis_test, 0, CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE);

static void async_listener_callback(const struct zbus_channel *chan, const void *message)
{
	static uint32_t count;

	if (chan != &chan_ping) {
		return;
	}

	const struct msg_ping *msg = message;

	if (msg->should_pong) {
		k_msgq_put(&msgq_pong, &count, K_MSEC(250));
	}
	++count;
}

ZBUS_ASYNC_LISTENER_DEFINE(alis1, async_listener_callback);

ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE(alis2, NULL, false);

static void isr_handler(const void *message)
{
	const struct msg_ping *msg = message;

	zbus_chan_pub(&chan_ping, msg, K_NO_WAIT);

	k_sem_give(&sem_alis_test);
}

static void isr_burst_handler(const void *message)
{
	const struct msg_ping *msg = message;

	for (int i = 0; i < CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE; ++i) {
		int err = zbus_chan_pub(&chan_ping, msg, K_NO_WAIT);

		if (err) {
			break;
		}
	}

	k_sem_give(&sem_alis_test);
}

#define F(i, _) 4 + i

ZTEST(async_listener, test_01_specification)
{
	struct msg_ping msg;

	msg.should_pong = true;
	/* Add data to the msgq from thread */
	zbus_chan_pub(&chan_ping, &msg, K_SECONDS(1));

	msg.should_pong = false;
	/* Do not add data to the msgq from thread */
	zbus_chan_pub(&chan_ping, &msg, K_SECONDS(1));

	/* Publications from ISR must have the same behavior as the previous tests */
	msg.should_pong = true;
	/* Add data to the msgq from ISR */
	irq_offload(isr_handler, &msg);

	msg.should_pong = false;
	/* Do not add data to the msgq from ISR */
	irq_offload(isr_handler, &msg);

	zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	k_msleep(250);

	msg.should_pong = true;
	/* Add data in burst mode to the msgq from ISR */
	irq_offload(isr_burst_handler, &msg);

	zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	k_msleep(250);

	int pong_count = 2 /* should_pong==true*/
			 + (CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE - 1) /* ISR burst */;

#if defined(CONFIG_SMP)
	/* That does not reach the limit of the pool size in SMP architectures */
	pong_count += 1;
#endif

	zassert_equal(k_msgq_num_used_get(&msgq_pong), pong_count);

	uint32_t expected_queue[] = {
		0, 2,
		LISTIFY(CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_POOL_SIZE, F, (,)) /* only for SMP */};

	uint32_t data;

	for (int i = 0; i < pong_count; ++i) {
		zassert_ok(k_msgq_get(&msgq_pong, &data, K_FOREVER));
		zassert_equal(expected_queue[i], data);
	}
	zassert_not_ok(k_msgq_get(&msgq_pong, &data, K_NO_WAIT));
}

struct msg_timestamp {
	uint32_t cycles;
	int64_t milliseconds;
};

ZBUS_CHAN_DEFINE(chan_u8,              /* Name */
		 struct msg_timestamp, /* Message type */

		 NULL,                         /* Validator */
		 NULL,                         /* User data */
		 ZBUS_OBSERVERS(alis2, alis3), /* observers */
		 ZBUS_MSG_INIT(0)              /* Initial value */
);

static struct msg_timestamp static_timestamp;

static void async_listener2_callback(const struct zbus_channel *chan, const void *message)
{
	const struct msg_timestamp *msg = message;

	zassert_equal(msg->cycles, static_timestamp.cycles);

	zassert_equal(msg->milliseconds, static_timestamp.milliseconds);

	zassert_mem_equal(k_thread_name_get(k_current_get()), "My work queue",
			  sizeof("My work queue"));

	k_sem_give(&sem_alis_test);
}

/* This second async listener uses an isolated work queue to process the async data. */
#define WQ_STACK_SIZE 1024
#define WQ_PRIORITY   5

K_THREAD_STACK_DEFINE(wq_stack_area, WQ_STACK_SIZE);
struct k_work_q my_work_q;

ZBUS_ASYNC_LISTENER_DEFINE(alis3, async_listener2_callback);

static void *setup(void)
{
	/* Steps necessary to set an isolated queue to an async listener: setup BEGIN */
	const struct k_work_queue_config cfg = {
		.name = "My work queue",
		.no_yield = false,
	};

	k_work_queue_init(&my_work_q);

	k_work_queue_start(&my_work_q, wq_stack_area, K_THREAD_STACK_SIZEOF(wq_stack_area),
			   WQ_PRIORITY, &cfg);

	zbus_async_listener_set_work_queue(&alis3, &my_work_q);
	/* setup END */

	return NULL;
}

ZTEST(async_listener, test_02_isolated_wqueue)
{
	k_sem_reset(&sem_alis_test);

	struct zbus_async_listener_work *async_listener =
		CONTAINER_OF(alis3.work, struct zbus_async_listener_work, work);

	zassert_not_equal(async_listener->queue, &k_sys_work_q);

	for (int i = 0; i < 5; ++i) {

		static_timestamp.cycles = k_cycle_get_32();
		static_timestamp.milliseconds = k_uptime_get();

		zbus_chan_pub(&chan_u8, &static_timestamp, K_FOREVER);

		zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
	}
}

ZTEST_SUITE(async_listener, NULL, setup, NULL, NULL, NULL);
