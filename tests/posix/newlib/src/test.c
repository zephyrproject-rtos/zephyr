/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdlib.h>
#include <posix/pthread.h>

static int test(void)
{
	int *res = NULL;
	int ret;
	pthread_attr_t attr;

	/* Using something from POSIX */
	ret = pthread_attr_init(&attr);

	/* Using something from libc */
	res = malloc(10);

	if ((res != NULL) && (ret == 0)) {
		TC_PRINT("\nhello world!\n");
	} else {
		return TC_FAIL;
	}

	free(res);

	/* We have gotten this far, so the build succeeded with both
	 * POSIX and newlib enabled.
	 */
	return TC_PASS;
}

void test_posix_newlib(void)
{
	zassert_true(test() == TC_PASS, NULL);
}
