/*
 * Copyright (c) 2020 Tobias Svehagen
 * Copyright (c) 2025 Atym, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "_main.h"

ZTEST_F(timerfd, test_expire_then_read)
{
	struct itimerspec ts = {
		.it_interval = {0, 100 * NSEC_PER_MSEC},
		.it_value = {0, 100 * NSEC_PER_MSEC},
	};
	timerfd_t val;
	int ret;

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));
	k_sleep(K_MSEC(550));

	ret = read(fixture->fd, &val, sizeof(val));
	zassert_true(ret == 8, "read ret %d", ret);
	zassert_true(val == 5, "val == %lld", val);
}

ZTEST_F(timerfd, test_not_started_shall_not_unblock)
{
	short event;
	int ret;

	event = POLLIN;
	ret = is_blocked(fixture->fd, &event);
	zassert_equal(ret, 1, "timerfd unblocked by expiry");
}

ZTEST_F(timerfd, test_poll_timeout)
{
	struct pollfd pfd;
	int ret;

	pfd.fd = fixture->fd;
	pfd.events = POLLIN;

	ret = poll(&pfd, 1, 500);
	zassert_true(ret == 0, "poll ret %d", ret);
}

ZTEST_F(timerfd, test_set_poll_event_block)
{
	reopen(&fixture->fd, CLOCK_MONOTONIC, 0);
	zassert_ok(ioctl(fixture->fd, TFD_IOC_SET_TICKS, (uint64_t)TESTVAL));
	timerfd_poll_set_common(fixture->fd);
}

ZTEST_F(timerfd, test_unset_poll_event_block)
{
	timerfd_poll_unset_common(fixture->fd);
}

K_THREAD_STACK_DEFINE(thread_stack, CONFIG_TEST_STACK_SIZE);
static struct k_thread thread;

static void thread_timerfd_read_1(void *arg1, void *arg2, void *arg3)
{
	uint64_t value;
	struct timerfd_fixture *fixture = arg1;

	zassert_equal(read(fixture->fd, &value, sizeof(value)), sizeof(value));
	zassert_equal(value, 1);
}

ZTEST_F(timerfd, test_read_then_expire_block)
{
	struct itimerspec ts = {
		.it_interval = {0, 100 * NSEC_PER_MSEC},
		.it_value = {0, 100 * NSEC_PER_MSEC},
	};

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
			thread_timerfd_read_1, fixture, NULL, NULL, 0, 0, K_NO_WAIT);

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));

	k_thread_join(&thread, K_FOREVER);
}

static void thread_timerfd_close(void *arg1, void *arg2, void *arg3)
{
	struct timerfd_fixture *fixture = arg1;

	zassert_ok(close(fixture->fd));
}

ZTEST_F(timerfd, test_read_then_close_block)
{
   	timerfd_t value;

	struct itimerspec ts = {
		.it_interval = {1, 0},
		.it_value = {1, 0},
	};

	k_thread_create(&thread, thread_stack, K_THREAD_STACK_SIZEOF(thread_stack),
			thread_timerfd_close, fixture, NULL, NULL, 0, 0, K_MSEC(100));

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));

	zassert_equal(read(fixture->fd, &value, sizeof(value)), -1);

	k_thread_join(&thread, K_FOREVER);
}

ZTEST_F(timerfd, test_expire_while_pollin)
{
   	struct itimerspec ts = {
		.it_value = {0, 100 * NSEC_PER_MSEC},
	};

	struct zsock_pollfd fds[] = {
		{
			.fd = fixture->fd,
			.events = ZSOCK_POLLIN,
		},
	};
	timerfd_t value;
	int ret;

	zassert_ok(timerfd_settime(fixture->fd, 0, &ts, NULL));

	/* Expect 1 event */
	ret = zsock_poll(fds, ARRAY_SIZE(fds), 200);
	zassert_equal(ret, 1);

	zassert_equal(fds[0].revents, ZSOCK_POLLIN);

	/* Check value */
	zassert_equal(read(fixture->fd, &value, sizeof(value)), sizeof(value));
	zassert_equal(value, 1);
}
