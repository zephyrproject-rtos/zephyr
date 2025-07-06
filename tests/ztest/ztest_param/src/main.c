/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <stdlib.h>

ZTEST_SUITE(ztest_params, NULL, NULL, NULL, NULL, NULL);

ZTEST(ztest_params, test_dummy)
{
	printf("Dummy test\n");
}

ZTEST_P(ztest_params, test_int_param)
{
	if (data != NULL) {
		printf("Passed int value:%d\n", atoi(data));
	} else {
		printf("Run without parameter\n");
	}
}

ZTEST_P(ztest_params, test_string_param)
{
	if (data != NULL) {
		printf("Passed string:%s\n", (char *)data);
	} else {
		printf("Run without parameter\n");
	}
}
