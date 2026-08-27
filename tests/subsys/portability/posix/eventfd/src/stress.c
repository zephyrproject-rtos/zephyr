/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_main.h"

/* update interval for printing stats */
#if CONFIG_TEST_DURATION_S >= 60
#define UPDATE_INTERVAL_S 10
#elif CONFIG_TEST_DURATION_S >= 30
#define UPDATE_INTERVAL_S 5
#else
#define UPDATE_INTERVAL_S 1
#endif

enum th_id {
	WRITER,
	READER,
};

typedef int (*eventfd_op_t)(int fd);

static size_t count[2];
static struct k_thread th[2];
static const char *msg[2] = {
	[READER] = "reads",
	[WRITER] = "writes",
};

static int read_op(int fd);
static int write_op(int fd);

static const eventfd_op_t op[2] = {
	[READER] = read_op,
	[WRITER] = write_op,
};
static K_THREAD_STACK_ARRAY_DEFINE(th_stack, 2, CONFIG_TEST_STACK_SIZE);

static int read_op(int fd)
{
	eventfd_t value;

	return eventfd_read(fd, &value);
}

static int write_op(int fd)
{
	return eventfd_write(fd, 1);
}

static void th_fun(void *arg1, void *arg2, void *arg3)
{

	int ret;
	uint64_t now;
	uint64_t end;
	uint64_t report;
	enum th_id id = POINTER_TO_UINT(arg1);
	struct eventfd_fixture *fixture = arg2;
	const uint64_t report_ms = UPDATE_INTERVAL_S * MSEC_PER_SEC;
	const uint64_t end_ms = CONFIG_TEST_DURATION_S * MSEC_PER_SEC;

	for (now = k_uptime_get(), end = now + end_ms, report = now + report_ms; now < end;
	     now = k_uptime_get()) {

		ret = op[id](fixture->fd);
		if (IS_ENABLED(CONFIG_TEST_EXTRA_ASSERTIONS)) {
			zassert_true(ret == 0 || (ret == -1 && errno == EAGAIN),
				     "ret: %d errno: %d", ret, errno);
		}
		count[id] += (ret == 0);

		if (!IS_ENABLED(CONFIG_TEST_EXTRA_QUIET)) {
			if (now >= report) {
				printk("%zu %s\n", count[id], msg[id]);
				report += report_ms;
			}
		}
		Z_SPIN_DELAY(10);
	}

	printk("avg: %zu %s/s\n", (size_t)((count[id] * MSEC_PER_SEC) / end_ms), msg[id]);
}

ZTEST_F(eventfd, test_stress)
{
	enum th_id i;
	enum th_id begin = MIN(READER, WRITER);
	enum th_id end = MAX(READER, WRITER) + 1;

	printk("BOARD: %s\n", CONFIG_BOARD);
	printk("TEST_DURATION_S: %u\n", CONFIG_TEST_DURATION_S);
	printk("UPDATE_INTERVAL_S: %u\n", UPDATE_INTERVAL_S);

	reopen(&fixture->fd, 0, EFD_NONBLOCK | EFD_SEMAPHORE);

	for (i = begin; i < end; ++i) {
		k_thread_create(&th[i], th_stack[i], K_THREAD_STACK_SIZEOF(th_stack[0]), th_fun,
				UINT_TO_POINTER(i), fixture, NULL, K_LOWEST_APPLICATION_THREAD_PRIO,
				0, K_NO_WAIT);
	}

	for (i = begin; i < end; ++i) {
		zassert_ok(k_thread_join(&th[i], K_FOREVER));
	}

	zassert_true(count[READER] > 0, "read count is zero");
	zassert_true(count[WRITER] > 0, "write count is zero");
	zassert_true(count[WRITER] >= count[READER], "read count (%zu) > write count (%zu)",
		     count[READER], count[WRITER]);
}
