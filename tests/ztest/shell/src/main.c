/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>

static struct ztest_shell_fixture {
	char parameter[128];
} suite_fixture = { .parameter = {0} };

static int cmd_set_parameter(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		strncpy(suite_fixture.parameter, argv[1], sizeof(suite_fixture.parameter) - 1);
		/* Ensure null termination */
		suite_fixture.parameter[sizeof(suite_fixture.parameter) - 1] = 0;
		shell_print(sh, "Parameter set to: %s", suite_fixture.parameter);
	} else {
		shell_print(sh, "Empty parameter, clearing current value");
		/* Making the parameter empty */
		suite_fixture.parameter[0] = 0;
	}

	return 0;
}

static void *suite_setup(void)
{
	/* Ensure null termination */
	suite_fixture.parameter[sizeof(suite_fixture.parameter) - 1] = 0;
	return &suite_fixture;
}

SHELL_CMD_ARG_REGISTER(set_parameter, NULL, "Set parameter for ztest_shell test cases",
cmd_set_parameter, 0, 1);
/* ^ I know it's formatted ugly, but the CI compliance check bullied me into this. */

ZTEST_SUITE(ztest_shell, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(ztest_shell, test_dummy)
{
	printf("Dummy test\n");
}

ZTEST_F(ztest_shell, test_int_param)
{
	if (strlen(fixture->parameter) > 0) {
		char *endPtr;
		int intParameter = strtol(fixture->parameter, &endPtr, 10);

		zassert_equal(*endPtr, 0, "Parameter is not an integer: %s", fixture->parameter);
		printf("Passed integer: %d\n", intParameter);
	} else {
		printf("Run without parameter\n");
	}
}

ZTEST_F(ztest_shell, test_string_param)
{
	if (strlen(fixture->parameter) > 0) {
		printf("Passed string: %s\n", fixture->parameter);
	} else {
		printf("Run without parameter\n");
	}
}
