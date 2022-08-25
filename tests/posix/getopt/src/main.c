/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive getopt test suite
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <string.h>
#include <getopt.h>

ZTEST_SUITE(getopt_test_suite, NULL, NULL, NULL, NULL, NULL);

ZTEST(getopt_test_suite, test_getopt_basic)
{
	static const char *const nargv[] = {
		"cmd_name",
		"-b",
		"-a",
		"-h",
		"-c",
		"-l",
		"-h",
		"-a",
		"-i",
		"-w",
	};
	static const char *accepted_opt = "abchw";
	static const char *expected = "bahc?ha?w";
	size_t argc = ARRAY_SIZE(nargv);
	int cnt = 0;
	int c;
	char **argv;

	argv = (char **)nargv;

	/* Get state of the current thread */
	getopt_init();

	do {
		c = getopt(argc, argv, accepted_opt);
		if (cnt >= strlen(expected)) {
			break;
		}

		zassert_equal(c, expected[cnt++], "unexpected opt character");
	} while (c != -1);

	c = getopt(argc, argv, accepted_opt);
	zassert_equal(c, -1, "unexpected opt character");
}

enum getopt_idx {
	GETOPT_IDX_CMD_NAME,
	GETOPT_IDX_OPTION1,
	GETOPT_IDX_OPTION2,
	GETOPT_IDX_OPTARG
};

ZTEST(getopt_test_suite, test_getopt)
{
	struct getopt_state *state;
	static const char *test_opts = "ac:";
	static const char *const nargv[] = {
		[GETOPT_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_IDX_OPTION1] = "-a",
		[GETOPT_IDX_OPTION2] = "-c",
		[GETOPT_IDX_OPTARG] = "foo",
	};
	int argc = ARRAY_SIZE(nargv);
	char **argv;
	int c;

	/* Get state of the current thread */
	getopt_init();

	argv = (char **)nargv;

	/* Test uknown option */

	c = getopt(argc, argv, test_opts);
	zassert_equal(c, 'a', "unexpected opt character");
	c = getopt(argc, argv, test_opts);
	zassert_equal(c, 'c', "unexpected opt character");

	c = getopt(argc, argv, test_opts);
	state = getopt_state_get();

	/* Thread safe usge: */
	zassert_equal(0, strcmp(argv[GETOPT_IDX_OPTARG], state->optarg),
		      "unexpected optarg result");
	/* Non thread safe usage: */
	zassert_equal(0, strcmp(argv[GETOPT_IDX_OPTARG], optarg),
		      "unexpected optarg result");
}

enum getopt_long_idx {
	GETOPT_LONG_IDX_CMD_NAME,
	GETOPT_LONG_IDX_VERBOSE,
	GETOPT_LONG_IDX_OPT,
	GETOPT_LONG_IDX_OPTARG
};

ZTEST(getopt_test_suite, test_getopt_long)
{
	/* Below test is based on example
	 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	 */
	struct getopt_state *state;
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	char **argv;
	int c;
	struct option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		 * We distinguish them by their indices.
		 */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	static const char *accepted_opt = "ac:d:e:";

	static const char *const argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	static const char *const argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	static const char *const argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	static const char *const argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should not be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	/* Get state of the current thread */
	getopt_init();
	argv = (char **)argv1;
	c = getopt_long(argc1, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	c = getopt_long(argc1, argv, accepted_opt, long_options, &option_index);
	state = getopt_state_get();
	zassert_equal('c', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long(argc1, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "getopt_long shall return -1");

	/* Test scenario 2 */
	argv = (char **)argv2;
	getopt_init();
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal('d', c, "unexpected option");
	state = getopt_state_get();
	zassert_equal(0, strcmp(state->optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "getopt_long shall return -1");

	/* Test scenario 3 */
	argv = (char **)argv3;
	getopt_init();
	c = getopt_long(argc3, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc3, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal('a', c, "unexpected option");
	c = getopt_long(argc3, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "getopt_long shall return -1");

	/* Test scenario 4 */
	argv = (char **)argv4;
	getopt_init();
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);
	/* Function was called with option '-l'. It is expected it will be
	 * NOT evaluated to '--long' which has flag 'e'.
	 */
	zassert_not_equal('e', c, "unexpected option match");
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);
}

ZTEST(getopt_test_suite, test_getopt_long_only)
{
	/* Below test is based on example
	 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	 */
	struct getopt_state *state;
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	char **argv;
	int c;
	struct option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		 * We distinguish them by their indices.
		 */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	static const char *accepted_opt = "ac:d:e:";

	static const char *const argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	static const char *const argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	static const char *const argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	static const char *const argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	argv = (char **)argv1;
	getopt_init();
	c = getopt_long_only(argc1, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	c = getopt_long_only(argc1, argv, accepted_opt,
			     long_options, &option_index);
	state = getopt_state_get();
	zassert_equal('c', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long_only(argc1, argv, accepted_opt,
				   long_options, &option_index);
	zassert_equal(-1, c, "getopt_long_only shall return -1");

	/* Test scenario 2 */
	argv = (char **)argv2;
	getopt_init();
	state = getopt_state_get();
	c = getopt_long_only(argc2, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc2, argv, accepted_opt,
				   long_options, &option_index);
	state = getopt_state_get();
	zassert_equal('d', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long_only(argc2, argv, accepted_opt,
				   long_options, &option_index);
	zassert_equal(-1, c, "getopt_long_only shall return -1");

	/* Test scenario 3 */
	argv = (char **)argv3;
	getopt_init();
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal('a', c, "unexpected option");
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(-1, c, "getopt_long_only shall return -1");

	/* Test scenario 4 */
	argv = (char **)argv4;
	getopt_init();
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);

	/* Function was called with option '-l'. It is expected it will be
	 * evaluated to '--long' which has flag 'e'.
	 */
	zassert_equal('e', c, "unexpected option");
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);
}
