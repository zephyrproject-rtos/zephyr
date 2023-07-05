/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <errno.h>
#include <unistd.h>

ZTEST(posix_apis, test_posix_sysconf)
{
	int rc;

	/* SC that's implemented */
	rc = sysconf(_SC_VERSION);
	zassert_equal(rc, 200809L, "sysconf returned unexpected value %d", rc);
	zassert_equal(errno, 0, "errno should be %s instead of %d", "0", errno);

	/* SC that's not implemented */
	rc = sysconf(_SC_2_C_BIND);
	zassert_equal(rc, -1, "sysconf returned unexpected value %d", rc);
	zassert_equal(errno, ENOSYS, "errno should be %s instead of %d", "ENOSYS", errno);

	/* Random value */
	rc = sysconf(0xdeadbeef);
	zassert_equal(rc, -1, "sysconf returned unexpected value %d", rc);
	zassert_equal(errno, EINVAL, "errno should be %s instead of %d", "EINVAL", errno);
}
