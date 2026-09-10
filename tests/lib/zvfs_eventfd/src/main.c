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
