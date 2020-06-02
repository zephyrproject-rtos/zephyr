/*
 * Copyright (c) 2020 Tobias Svehagen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 0
#endif

#include <net/socket.h>

#include <poll.h>

#include <sys/eventfd.h>

#define EVENTFD_STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE + \
				 PTHREAD_STACK_MIN)

K_THREAD_STACK_DEFINE(eventfd_stack, EVENTFD_STACK_SIZE);
static pthread_t eventfd_thread;

static void test_eventfd(void)
{
	int fd = eventfd(0, 0);

	zassert_true(fd >= 0, "fd == %d", fd);

	close(fd);
}

static void test_eventfd_read_nonblock(void)
{
	eventfd_t val;
	int fd, ret;

	fd = eventfd(0, EFD_NONBLOCK);
	zassert_true(fd >= 0, "fd == %d", fd);

	ret = eventfd_read(fd, &val);
	zassert_true(ret == -1, "read ret %d", ret);
	zassert_true(errno == EAGAIN, "errno %d", errno);

	close(fd);
}

static void test_eventfd_write_then_read(void)
{
	eventfd_t val;
	int fd, ret;

	fd = eventfd(0, 0);
	zassert_true(fd >= 0, "fd == %d", fd);

	ret = eventfd_write(fd, 3);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_write(fd, 2);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_read(fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 5, "val == %d", val);

	close(fd);

	/* Test EFD_SEMAPHORE */

	fd = eventfd(0, EFD_SEMAPHORE);
	zassert_true(fd >= 0, "fd == %d", fd);

	ret = eventfd_write(fd, 3);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_write(fd, 2);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_read(fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 1, "val == %d", val);

	close(fd);
}

static void test_eventfd_poll_timeout(void)
{
	struct pollfd pfd;
	int fd, ret;

	fd = eventfd(0, 0);
	zassert_true(fd >= 0, "fd == %d", fd);

	pfd.fd = fd;
	pfd.events = POLLIN;

	ret = poll(&pfd, 1, 500);
	zassert_true(ret == 0, "poll ret %d", ret);

	close(fd);
}

struct test_thread_data {
	int fd;
	struct k_sem written_twice;
};

static void *test_thread_wait_and_write(void *arg)
{
	struct test_thread_data *d = arg;
	int fd = d->fd;
	int ret;

	k_sleep(K_MSEC(500));

	ret = eventfd_write(fd, 10);
	zassert_true(ret == 0, "write ret %d", ret);

	ret = eventfd_write(fd, 10);
	zassert_true(ret == 0, "write ret %d", ret);

	k_sem_give(&d->written_twice);

	k_sleep(K_MSEC(1500));

	ret = eventfd_write(fd, 5);
	zassert_true(ret == 0, "write ret %d", ret);

	return NULL;
}

static void test_eventfd_poll_event(void)
{
	struct sched_param schedparam;
	pthread_attr_t attr;
	struct pollfd pfd;
	eventfd_t val;
	struct test_thread_data d;
	int fd, ret;

	fd = eventfd(0, 0);
	zassert_true(fd >= 0, "fd == %d", fd);

	ret = pthread_attr_init(&attr);
	zassert_true(ret == 0, "pthread_attr_init ret %d", ret);

	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	zassert_true(ret == 0, "pthread_attr_setschedpolicy ret %d", ret);

	schedparam.sched_priority = 1;
	ret = pthread_attr_setschedparam(&attr, &schedparam);
	zassert_true(ret == 0, "pthread_attr_setschedparam ret %d", ret);

	ret = pthread_attr_setstack(&attr, eventfd_stack, EVENTFD_STACK_SIZE);
	zassert_true(ret == 0, "pthread_attr_setstack ret %d", ret);

	d.fd = fd;
	k_sem_init(&d.written_twice, 0, 1);

	ret = pthread_create(&eventfd_thread, &attr, test_thread_wait_and_write,
			     &d);
	zassert_true(ret == 0, "pthread_create ret %d", ret);

	pfd.fd = fd;
	pfd.events = POLLIN;

	TC_PRINT("Waiting for first event\n");

	ret = poll(&pfd, 1, 3000);
	zassert_true(ret == 1, "poll ret %d %d", ret, pfd.revents);
	zassert_equal(pfd.revents, POLLIN, "POLLIN not set");

	k_sem_take(&d.written_twice, K_FOREVER);

	TC_PRINT("Reading two events\n");

	ret = eventfd_read(fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 20, "val == %d", val);

	TC_PRINT("Waiting 1s for event after 1.5s (should timeout)\n");

	ret = poll(&pfd, 1, 1000);
	zassert_true(ret == 0, "poll ret %d %d", ret, pfd.revents);
	zassert_equal(pfd.revents, 0, "POLLIN set");

	TC_PRINT("Waiting 3s for event after 0.5s (should succeed)\n");

	ret = poll(&pfd, 1, 3000);
	zassert_true(ret == 1, "poll ret %d %d", ret, pfd.revents);
	zassert_equal(pfd.revents, POLLIN, "POLLIN not set");

	TC_PRINT("Reading last event\n");

	ret = eventfd_read(fd, &val);
	zassert_true(ret == 0, "read ret %d", ret);
	zassert_true(val == 5, "val == %d", val);

	close(fd);
}

void test_main(void)
{
	ztest_test_suite(test_eventfd,
				ztest_unit_test(test_eventfd),
				ztest_unit_test(test_eventfd_read_nonblock),
				ztest_unit_test(test_eventfd_write_then_read),
				ztest_unit_test(test_eventfd_poll_timeout),
				ztest_unit_test(test_eventfd_poll_event)
				);
	ztest_run_test_suite(test_eventfd);
}
