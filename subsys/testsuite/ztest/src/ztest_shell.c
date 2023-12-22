/*
 * Copyright 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <fnmatch.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/shell/shell.h>
#include <zephyr/ztest_test.h>

#ifdef CONFIG_TEST_ZTEST_SHELL
#define ZTEST_SHELL_STATIC
#else
#define ZTEST_SHELL_STATIC static
#endif

/* can be e.g. "gtest_" to be compatible with Google Test Framework */
#define PREFIX CONFIG_ZTEST_SHELL_PREFIX

#define N_ARGS (ARRAY_SIZE(ztest_shell_longopts) - 1)

struct ztest_shell_state {
	const struct shell *sh;
	const char *filter;
	const char *output;
	uint32_t seed;
	int32_t repeat;
	bool disabled: 1;
	bool help: 1;
	bool list_tests: 1;
	bool shuffle: 1;
	bool color: 1;
	bool time: 1;
} __packed;

static struct ztest_shell_state _state;

/* forward declarations */
extern const struct ztest_arch_api ztest_api;
static const char *const cmd_ztest_help;

void __ztest_shell_end_report(void);
ZTEST_SHELL_STATIC
int ztest_shell_filter(const char *test, const char *filter);

/* iterate over all suites */
typedef void (*ztest_shell_foreach_suite_cb_t)(const struct shell *sh,
					       const struct ztest_suite_node *suite,
					       void *user_data);
static void ztest_shell_foreach_suite(const struct shell *sh, ztest_shell_foreach_suite_cb_t cb,
				      void *user_data)
{
	STRUCT_SECTION_FOREACH(ztest_suite_node, suite) {
		cb(sh, suite, user_data);
	}
}

/* iterate over all tests */
typedef void (*ztest_shell_foreach_test_cb_t)(const struct shell *sh,
					      const struct ztest_unit_test *test, void *user_data);
static void ztest_shell_foreach_test(const struct shell *sh, ztest_shell_foreach_test_cb_t cb,
				     void *user_data)
{
	STRUCT_SECTION_FOREACH(ztest_unit_test, test) {
		cb(sh, test, user_data);
	}
}

static void ztest_shell_list_test(const struct shell *sh, const struct ztest_unit_test *test,
				  void *user_data)
{
	ARG_UNUSED(user_data);
	shell_print(sh, "%s.%s", test->test_suite_name, test->name);
}

static void ztest_shell_sum_suite_failures(const struct shell *sh,
					   const struct ztest_suite_node *suite, void *user_data)
{
	size_t *failures = user_data;
	*failures += suite->stats->fail_count;
}

static int ztest_shell_execute(const struct shell *sh, const struct ztest_shell_state *opts)
{
	size_t failures = 0;

	if (opts->help) {
		shell_print(sh, "%s", cmd_ztest_help);
		return 0;
	}

	if (opts->list_tests) {
		ztest_shell_foreach_test(sh, ztest_shell_list_test, (void *)opts);
		return 0;
	}

	__ASSERT_NO_MSG(opts == &_state);
	ztest_api.run_all(opts);
	__ztest_shell_end_report();
	ztest_shell_foreach_suite(sh, ztest_shell_sum_suite_failures, (void *)opts);

	return (failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

static const struct option ztest_shell_longopts[] = {
	{
		.name = "help",
		.val = 'h',
	},
	{
		.name = PREFIX "list_tests",
		.val = 'l',
	},
	{
		.name = PREFIX "filter",
		.has_arg = true,
		.val = 'f',
	},
	{
		.name = PREFIX "also_run_disabled_tests",
		.val = 'd',
	},
	{
		.name = PREFIX "repeat",
		.has_arg = true,
		.val = 'r',
	},
	{
		.name = PREFIX "shuffle",
		.val = 'u',
	},
	{
		.name = PREFIX "random_seed",
		.val = 's',
	},
	{
		.name = PREFIX "color",
		.has_arg = true,
		.val = 'c',
	},
	{
		.name = PREFIX "print_time",
		.has_arg = true,
		.val = 't',
	},
	{
		.name = PREFIX "output",
		.has_arg = true,
		.val = 'o',
	},
	{0},
};
static const char *const cmd_ztest_help =
	"Run Ztest test suites\n"
	"usage: ztest [arguments..]\n"
	"\n"
	"Without specifying any arguments, all tests from all testsuites are run.\n"
	"\n"
	"arguments:\n"
	"--" PREFIX "filter=POSTIVE_PATTERNS[-NEGATIVE_PATTERNS]\n"
	"                         run test that match POSITIVE_PATTERNS and do not match\n"
	"                         NEGATIVE_PATTERNS\n"
	"--help                   print this help message\n"
	"--" PREFIX "list_tests   list all test suites and tests\n"
	"--" PREFIX "also_run_disabled_tests\n"
	"                         also run disabled tests\n"
	"--" PREFIX "repeat       run tests repeatedly (use a negative number to repeat forever)\n"
	"--" PREFIX "shuffle      randomize test order on every iteration\n"
	"--" PREFIX "random_seed  seed for shuffling ([1,9999], 0 to seed based on current time)\n"
	"--" PREFIX "color=(yes|no|auto)\n"
	"                         enable / disable colored output. the default is off\n"
	"--" PREFIX "print_time=(0|1)\n"
	"                         print time duration for each test (defaults to 1)\n"
	"--" PREFIX "output=(json|xml)[:DIRECTORY_PATH/|:FILE_PATH]\n"
	"                         generate a json or xml report in the given directory or with\n"
	"                         the given filename. The default is not to generate a report\n"
	"\n"
	"See https://pastebin.com/GtBg0aHZ for details\n";

static int ztest_shell_parse_opts(const struct shell *sh, int argc, char *const *argv,
				  struct ztest_shell_state *opts)
{
	int ret;
	int option_index = 1;

	while ((ret = getopt_long(argc, argv, "", ztest_shell_longopts, &option_index)) != -1) {
		switch (ret) {
		case 'h':
			opts->help = true;
			break;
		case 'l':
			opts->list_tests = true;
			break;
		case 'f':
			opts->filter = optarg;
			break;
		case 'd':
			opts->disabled = true;
			shell_warn(sh, "running disabled tests is not yet implemented");
			break;
		case 'r':
			opts->repeat = atoi(optarg);
			break;
		case 'u':
			opts->shuffle = true;
			shell_warn(sh, "shuffle is not yet implemented");
			break;
		case 's':
			opts->seed = atoi(optarg);
			shell_warn(sh, "random_seed is not yet implemented");
			break;
		case 'c':
			opts->color = strcmp(optarg, "no") != 0;
			shell_warn(sh, "color is not yet implemented");
			break;
		case 't':
			opts->time = (bool)atoi(optarg);
			shell_warn(sh, "print_time is not yet implemented");
			break;
		case 'o':
			opts->output = optarg;
			shell_warn(sh, "writing output to file is not yet implemented");
			break;
		case '?':
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static int ztest_shell_verify_opts(const struct shell *sh, struct ztest_shell_state *opts)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(opts);

	return 0;
}

static int cmd_ztest(const struct shell *sh, size_t argc, char *const *argv)
{
	int ret;

	_state = (struct ztest_shell_state){
		.repeat = 1,
		.time = true,
		.sh = sh,
	};

	ret = ztest_shell_parse_opts(sh, argc, argv, &_state);
	if (ret < 0) {
		return ret;
	}

	ret = ztest_shell_verify_opts(sh, &_state);
	if (ret < 0) {
		return ret;
	}

	return ztest_shell_execute(sh, &_state);
}

SHELL_CMD_ARG_REGISTER(ztest, NULL, cmd_ztest_help, cmd_ztest, 0, N_ARGS);

bool z_ztest_should_test_run(const char *suite, const char *test)
{
	char name[CONFIG_ZTEST_SHELL_NAME_SIZE_MAX];
	size_t name_size = strlen(suite) + 1 /* '.' */ + strlen(test) + 1 /* '\0' */;

	if (name_size > CONFIG_ZTEST_SHELL_NAME_SIZE_MAX) {
		printk("Warning: CONFIG_ZTEST_SHELL_NAME_SIZE_MAX is not large enough for\n");
		printk("Warning: %s.%s\n", suite, test);
		printk("Warning: Needed: %zu\n", name_size);
		printk("Warning: Actual: %zu\n", (size_t)CONFIG_ZTEST_SHELL_NAME_SIZE_MAX);
		return false;
	}

	snprintf(name, sizeof(name), "%s.%s", suite, test);

	return ztest_shell_filter(name, _state.filter) > 0;
}

/**
 * @brief Filter ZTests by name.
 *
 * Filter the named @p test (interpreted as `suite.test`) through filters
 * specified via @p filter. This functionality is inspired by Google Test, where test
 * executables accept the `--gtest_filter=POSITIVE_PATTERNS[-NEGATIVE_PATTERNS]` CLI option.
 *
 * A test should be run (i.e. passes the filter) if it has a name that matches at least one of
 * `POSITIVE_PATTERNS` but matches precisely zero `NEGATIVE_PATTERNS`. A test should not be run
 * (i.e. is caught by the filter) if it has a name that matches precisely zero `POSITIVE_PATTERNS`
 * or at least one `NEGATIVE_PATTERNS`.
 *
 * Patterns may include a question mark `?`, which matches a single character, or an asterisk (`*`)
 * which matches any substring. Multiple patterns may be separated by a colon (`:`).
 *
 * @param test The name of the ZTest
 * @param filter positive and negative patterns
 *
 * @retval 0 if the test should not be run
 * @retval 1 if the test should be run
 * @retval -EINVAL if an argument is invalid.
 * @retval -ENOMEM if there is insufficient memory
 *
 * @see also tests/unit/ztest/shell/main.c
 */
ZTEST_SHELL_STATIC
int ztest_shell_filter(const char *test, const char *filter)
{
	int ret;
	size_t filter_len;
	size_t pmatch_count = 0;
	/* evaluate matches as positive until a '-' is seen */
	bool positive_match = true;
	char pattern[CONFIG_ZTEST_SHELL_PATTERN_MAX];

	if (test == NULL || strlen(test) == 0) {
		return -EINVAL;
	}

	if (filter == NULL || strlen(filter) == 0) {
		return (int)true;
	}

	/* a sliding window over patterns */
	bool found_pattern = false;

	pattern[0] = '\0';
	/* include the '\0' terminator */
	filter_len = strlen(filter) + 1;
	for (size_t a = 0, b = 0; a < filter_len && b <= filter_len;) {
		switch (filter[b]) {
		case '\0':
		case ':':
			found_pattern = true;
			if (b - a < sizeof(pattern)) {
				pattern[b - a] = '\0';
			}
			break;
		case '-':
			if (!positive_match) {
				/* we already encountered a '-', invalid filter! */
				return -EINVAL;
			}
			if (a == 0 && b == 0) {
				/* when filter[0] == '-' implies '*' in POSITIVE_PATTERNS */
				++pmatch_count;
				/* reset filter and switch to negative match */
				positive_match = false;
				goto reset;
			} else {
				found_pattern = true;
				if (b - a < sizeof(pattern)) {
					pattern[b - a] = '\0';
				}
			}
			break;
		default:
			if (b - a < sizeof(pattern)) {
				pattern[b - a] = filter[b];
			}
			break;
		}

		if (!found_pattern) {
			++b;
			continue;
		}

		if (pattern[0] == '\0') {
			/* ignore empty patterns found in the middle or at the end of a filter */
			goto reset;
		}

		if (b - a >= sizeof(pattern)) {
			printk("Warning: CONFIG_ZTEST_SHELL_PATTERN_MAX is not large enough for\n");
			printk("Warning: %.*s\n", (int)(b - a), &filter[a]);
			printk("Warning: Needed: %zu\n", b - a + 1);
			printk("Warning: Actual: %zu\n", (size_t)CONFIG_ZTEST_SHELL_PATTERN_MAX);
			return -ENOMEM;
		}

		ret = fnmatch(pattern, test, FNM_NOESCAPE);
		if (ret != 0 && ret != FNM_NOMATCH) {
			return -EINVAL;
		}
		if (positive_match && ret == 0) {
			++pmatch_count;
		} else if (!positive_match && ret == 0) {
			/* even 1 negative match can be shortcut to failure */
			return (int)false;
		}

		if (filter[b] == '-') {
			positive_match = false;
		}

reset:
		/* reset search for pattern */
		a = b + 1;
		b = a;
		pattern[0] = '\0';
		found_pattern = false;
	}

	return pmatch_count > 0;
}
