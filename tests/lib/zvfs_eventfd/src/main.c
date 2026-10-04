/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/zvfs/eventfd.h>

#define ADD_SIZE_SUM (CONFIG_ZVFS_EVENTFD_ADD_SIZE_TEST_A + CONFIG_ZVFS_EVENTFD_ADD_SIZE_TEST_B)

#ifdef CONFIG_ZVFS_EVENTFD_IGNORE_MIN
#define EXPECTED_EVENTFD_SIZE CONFIG_ZVFS_EVENTFD_MAX
#else
#define EXPECTED_EVENTFD_SIZE MAX(CONFIG_ZVFS_EVENTFD_MAX, ADD_SIZE_SUM)
#endif

BUILD_ASSERT(ZVFS_EVENTFD_SIZE == EXPECTED_EVENTFD_SIZE,
	     "ZVFS_EVENTFD_SIZE does not match the expected eventfd count");

/* The eventfd count drives the size of the file descriptor table, so the table
 * must be large enough to hold exactly the configured number of eventfds.
 */
ZTEST(zvfs_eventfd_size, test_eventfd_size_matches_kconfig)
{
	zassert_equal(ZVFS_EVENTFD_SIZE, EXPECTED_EVENTFD_SIZE,
		      "Unexpected ZVFS_EVENTFD_SIZE: %d", ZVFS_EVENTFD_SIZE);
}

ZTEST(zvfs_eventfd_size, test_eventfd_exhaustion)
{
	int fds[ZVFS_EVENTFD_SIZE];
	int extra;

	/* All eventfd slots should be allocatable. */
	for (int i = 0; i < ZVFS_EVENTFD_SIZE; i++) {
		fds[i] = zvfs_eventfd(0, ZVFS_EFD_NONBLOCK);
		zassert_true(fds[i] >= 0, "Failed to allocate eventfd %d (errno %d)", i, errno);
	}

	/* The next allocation must fail once all eventfds are in use. */
	errno = 0;
	extra = zvfs_eventfd(0, ZVFS_EFD_NONBLOCK);
	zassert_equal(extra, -1, "Allocated more eventfds than configured");
	zassert_equal(errno, ENOMEM, "Unexpected errno: %d", errno);

	for (int i = 0; i < ZVFS_EVENTFD_SIZE; i++) {
		zassert_ok(zvfs_close(fds[i]), "Failed to close eventfd %d", i);
	}
}

ZTEST_SUITE(zvfs_eventfd_size, NULL, NULL, NULL, NULL, NULL);

K_THREAD_STACK_DEFINE(reader_stack, CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread reader_thread;
static volatile bool reader_entered;
static volatile bool reader_done;
static int reader_res;
static int reader_errno;
static zvfs_eventfd_t reader_value;

static void reader_cb(void *p1, void *p2, void *p3)
{
	int fd = POINTER_TO_INT(p1);

	ARG_UNUSED(p3);

	reader_entered = true;
	if (p2 != NULL) {
		reader_res = zvfs_read(fd, &reader_value, sizeof(reader_value), NULL);
	} else {
		reader_res = zvfs_eventfd_read(fd, &reader_value);
	}
	reader_errno = errno;
	reader_done = true;
}

static void start_reader(int fd, bool posix_read, int prio_offset)
{
	reader_entered = false;
	reader_done = false;
	reader_value = 0;

	k_thread_create(&reader_thread, reader_stack, K_THREAD_STACK_SIZEOF(reader_stack),
			reader_cb, INT_TO_POINTER(fd), posix_read ? INT_TO_POINTER(1) : NULL, NULL,
			k_thread_priority_get(k_current_get()) + prio_offset, 0, K_NO_WAIT);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* The windows below are made by priorities, which order threads on one CPU only */
	if (arch_num_cpus() > 1) {
		ztest_test_skip();
	}
}

ZTEST(zvfs_eventfd_close, test_eventfd_read_queued_during_close)
{
	const struct fd_op_vtable *vtable;
	struct k_mutex *lock;
	zvfs_eventfd_t value;

	int fd = zvfs_eventfd(1, 0);

	zassert_true(fd >= 0, "fd < 0");

	zassert_not_null(zvfs_get_fd_obj_and_vtable(fd, &vtable, &lock));
	zassert_ok(k_mutex_lock(lock, K_FOREVER));

	/* A higher priority reader runs on the yield and blocks on the entry lock */
	start_reader(fd, false, -1);
	k_yield();
	zassert_true(reader_entered, "reader did not run");
	zassert_false(reader_done, "reader did not block on the entry lock");

	zassert_ok(zvfs_close(fd));

	int other = zvfs_eventfd(5, 0);

	zassert_true(other >= 0, "other < 0");
	zassert_not_equal(other, fd, "slot reused while eventfd_read() waits on its lock");

	k_mutex_unlock(lock);
	zassert_ok(k_thread_join(&reader_thread, K_FOREVER));

	zassert_equal(reader_res, -1);
	zassert_equal(reader_errno, EBADF);
	zassert_ok(zvfs_eventfd_read(other, &value));
	zassert_equal(value, 5, "the new eventfd was read");
	zassert_ok(zvfs_close(other));
}

static void reader_woken_by_close(bool posix_read)
{
	zvfs_eventfd_t value;

	int fd = zvfs_eventfd(0, 0);

	zassert_true(fd >= 0, "fd < 0");

	void *obj = zvfs_get_fd_obj(fd, NULL, 0);

	/* A lower priority reader blocks waiting for a write */
	start_reader(fd, posix_read, 1);
	k_msleep(10);
	zassert_true(reader_entered, "reader did not run");
	zassert_false(reader_done, "reader did not block");

	/* close() wakes the reader, which runs only once we sleep */
	zassert_ok(zvfs_close(fd));

	int other = zvfs_eventfd(0, 0);

	zassert_true(other >= 0, "other < 0");
	zassert_equal_ptr(zvfs_get_fd_obj(other, NULL, 0), obj, "eventfd object not reused");
	zassert_ok(zvfs_eventfd_write(other, 1));

	zassert_ok(k_thread_join(&reader_thread, K_MSEC(100)));

	zassert_equal(reader_res, -1, "woken reader read the new eventfd");
	zassert_equal(reader_errno, EBADF);
	zassert_ok(zvfs_eventfd_read(other, &value));
	zassert_equal(value, 1);
	zassert_ok(zvfs_close(other));
}

ZTEST(zvfs_eventfd_close, test_eventfd_read_woken_by_close)
{
	reader_woken_by_close(false);
}

ZTEST(zvfs_eventfd_close, test_read_woken_by_close)
{
	reader_woken_by_close(true);
}

ZTEST_SUITE(zvfs_eventfd_close, NULL, NULL, before, NULL, NULL);
