/*
 * Copyright (c) 2020 Tobias Svehagen
 * Copyright (c) 2020 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include <zephyr/net/socket.h>

#ifdef CONFIG_POSIX_API
#include <sys/eventfd.h>
#else
#include <zephyr/posix/sys/eventfd.h>
#endif

ZTEST_SUITE(test_eventfd_basic, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_eventfd_basic, test_eventfd)
{
	int fd = eventfd(0, 0);

	zassert_true(fd >= 0, "fd == %d", fd);

	zsock_close(fd);
}

ZTEST(test_eventfd_basic, test_eventfd_write_then_read)
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

	zsock_close(fd);

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

	zsock_close(fd);
}
