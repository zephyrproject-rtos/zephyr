/*
 * Copyright (c) 2025 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct msg_bool {
	bool is_valid;
};

ZBUS_CHAN_DEFINE(chan_int,        /* Name */
		 struct msg_bool, /* Message type */

		 NULL,                            /* Validator */
		 NULL,                            /* User data */
		 ZBUS_OBSERVERS(alis1, alis2),    /* observers */
		 ZBUS_MSG_INIT(.is_valid = false) /* Initial value */
);

K_SEM_DEFINE(sem_alis_test, 0, CONFIG_ZBUS_ASYNC_LISTENER_MSG_BUF_POOL_SIZE_ADD);

static void async_listener_callback(const struct zbus_channel *chan, const void *message)
{
	if (chan != &chan_int) {
		return;
	}

	const struct msg_bool *msg = message;

	if (msg->is_valid) {
		k_sem_give(&sem_alis_test);
	}
}

ZBUS_ASYNC_LISTENER_DEFINE(alis1, async_listener_callback);

ZBUS_ASYNC_LISTENER_DEFINE_WITH_ENABLE(alis2, NULL, false);

static void isr_handler(const void *message)
{
	const struct msg_bool *msg = message;

	zbus_chan_pub(&chan_int, msg, K_NO_WAIT);
}

static void isr_burst_handler(const void *ptr)
{
	ARG_UNUSED(ptr);

	struct msg_bool msg = {.is_valid = true};

	int expected_failing_i = CONFIG_ZBUS_ASYNC_LISTENER_MSG_BUF_POOL_SIZE_ADD - 1;

	for (int i = 0; i < CONFIG_ZBUS_ASYNC_LISTENER_MSG_BUF_POOL_SIZE_ADD; ++i) {
		int err = zbus_chan_pub(&chan_int, &msg, K_NO_WAIT);

		if (err) {
			zassert_equal(err, -ENOMEM);
			zassert_equal(i, expected_failing_i);
			return;
		}
	}
}

ZTEST(async_listener, test_specification)
{
	struct msg_bool msg = {.is_valid = true};

	zbus_chan_pub(&chan_int, &msg, K_SECONDS(1));
	zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	msg.is_valid = false;
	zbus_chan_pub(&chan_int, &msg, K_SECONDS(1));

	/* This must timeout the sem take because the x is not greater than zero */
	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	/* Publications from ISR must have the same behavior as the previous tests */
	msg.is_valid = true;
	irq_offload(isr_handler, &msg);
	zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	msg.is_valid = false;
	irq_offload(isr_handler, &msg);
	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	/* Enabling an invalid async listener */

	msg.is_valid = true;
	irq_offload(isr_burst_handler, NULL);
	/* We can only publish up to CONFIG_ZBUS_ASYNC_LISTENER_MSG_BUF_POOL_SIZE_ADD - 1 */
	for (int i = 0; i < (CONFIG_ZBUS_ASYNC_LISTENER_MSG_BUF_POOL_SIZE_ADD - 1); ++i) {
		zassert_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
	}
	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
}

ZBUS_CHAN_DEFINE(chan_u8, /* Name */
		 uint8_t, /* Message type */

		 NULL,                         /* Validator */
		 NULL,                         /* User data */
		 ZBUS_OBSERVERS(alis2, alis3), /* observers */
		 0                             /* Initial value */
);

static uint8_t count;

static void async_listener2_callback(const struct zbus_channel *chan, const void *message)
{
	const uint8_t *c = message;

	zassert_equal(*c, count);

	zassert_mem_equal(k_thread_name_get(k_current_get()), "My work queue",
			  sizeof("My work queue"));
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

ZTEST(async_listener, test_isolated_wqueue)
{
	k_sem_reset(&sem_alis_test);

	struct zbus_async_listener_work *async_listener =
		CONTAINER_OF(alis3.work, struct zbus_async_listener_work, work);

	zassert_not_equal(async_listener->queue, &k_sys_work_q);

	count = 200;
	zbus_chan_pub(&chan_u8, &count, K_FOREVER);

	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));

	count = 1;
	zbus_chan_pub(&chan_u8, &count, K_FOREVER);

	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
	zassert_not_ok(k_sem_take(&sem_alis_test, K_SECONDS(1)));
}

ZTEST_SUITE(async_listener, NULL, setup, NULL, NULL, NULL);
