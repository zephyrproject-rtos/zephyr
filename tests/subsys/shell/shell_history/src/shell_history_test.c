/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite
 *
 */

#include <zephyr.h>
#include <ztest.h>

#include <shell/shell_history.h>

#define HIST_BUF_SIZE 160
Z_SHELL_HISTORY_DEFINE(history, HIST_BUF_SIZE);

static void init_test_buf(uint8_t *buf, size_t len, uint8_t offset)
{
	for (int i = 0; i < len; i++) {
		buf[i] = offset + i;
	}
}

/**
 * Function tests getting line from history and compares it against expected
 * result.
 */
static void test_get(bool ok, bool up, uint8_t *exp_buf, uint16_t exp_len)
{
	bool res;
	uint8_t out_buf[HIST_BUF_SIZE];
	uint16_t out_len;

	out_len = sizeof(out_buf);

	res = z_shell_history_get(&history, up, out_buf, &out_len);

	if (ok) {
		zassert_true(res, "history should contain one entry.\n");

		zassert_equal(out_len, exp_len, "Unexpected entry length.\n");
		if (out_len) {
			zassert_equal(memcmp(out_buf, exp_buf, out_len), 0,
				"Expected equal buffers.\n");
		}
	} else {
		zassert_false(res, "History should return nothing.\n");
	}
}

/* Test put line to history and get it.
 *
 * Test steps:
 * - initialize history.
 * - put line to the history.
 * - read line and verify that it is the one that was put.
 */
static void test_history_add_get(void)
{
	uint8_t exp_buf[HIST_BUF_SIZE];

	init_test_buf(exp_buf, sizeof(exp_buf), 0);

	z_shell_history_init(&history);

	test_get(false, true, NULL, 0);

	z_shell_history_put(&history, exp_buf, 20);

	test_get(true, true, exp_buf, 20);

	z_shell_history_purge(&history);
}

/* Test verifies that after purging there is no line in the history. */
static void test_history_purge(void)
{
	uint8_t exp_buf[HIST_BUF_SIZE];

	init_test_buf(exp_buf, sizeof(exp_buf), 0);

	z_shell_history_init(&history);

	z_shell_history_put(&history, exp_buf, 20);
	z_shell_history_put(&history, exp_buf, 20);

	z_shell_history_purge(&history);

	test_get(false, true, NULL, 0);
}

/* Test browsing history.
 *
 * Test steps:
 * - initialize history.
 * - put  lines 1,2,3 to history.
 * - get in up direction a line and verify that it's the last one added (3).
 * - get next line in up direction and verify that it's line 2.
 * - get next line in up direction and verify that it's line 1.
 * - get next line in down direction and verify that it's line 2.
 * - get next line in up direction and verify that it's line 1.
 * - get next line in down direction and verify that it's line 2.
 * - get next line in down direction and verify that it's line 3.
 * - attempt to get next line in down direction and verify that there is no
 *   line.
 */
static void test_history_get_up_and_down(void)
{
	uint8_t exp1_buf[HIST_BUF_SIZE];
	uint8_t exp2_buf[HIST_BUF_SIZE];
	uint8_t exp3_buf[HIST_BUF_SIZE];

	init_test_buf(exp1_buf, sizeof(exp1_buf), 0);
	init_test_buf(exp2_buf, sizeof(exp2_buf), 10);
	init_test_buf(exp3_buf, sizeof(exp3_buf), 20);

	z_shell_history_init(&history);

	z_shell_history_put(&history, exp1_buf, 20);
	z_shell_history_put(&history, exp2_buf, 15);
	z_shell_history_put(&history, exp3_buf, 20);

	test_get(true, true, exp3_buf, 20); /* up - 3*/
	test_get(true, true, exp2_buf, 15); /* up - 2*/
	test_get(true, true, exp1_buf, 20); /* up - 1*/
	test_get(true, false, exp2_buf, 15); /* down - 2 */
	test_get(true, true, exp1_buf, 20);  /* up - 1*/
	test_get(true, false, exp2_buf, 15); /* down - 2 */
	test_get(true, false, exp3_buf, 20); /* down - 3 */
	test_get(false, false, NULL, 0); /* down - nothing */

	z_shell_history_purge(&history);
}

/* Function for getting maximal buffer size that can be stored in the history */
static int get_max_buffer_len(void)
{
	uint8_t buf[HIST_BUF_SIZE];
	uint8_t out_buf[HIST_BUF_SIZE];
	int len = sizeof(buf);
	uint16_t out_len;

	z_shell_history_init(&history);

	do {
		z_shell_history_put(&history, buf, len);
		out_len = sizeof(out_buf);
		if (z_shell_history_get(&history, true, out_buf, &out_len)) {
			z_shell_history_purge(&history);
			break;
		}
	} while (len--);

	return len;
}

/* Test verifies that line that cannot fit into history buffer is not stored.
 *
 * Test steps:
 * - initialize history.
 * - put buffer that is bigger than history overall capacity.
 * - verify that history is empty.
 * - put short line followed by line that is close to max.
 * - verify that long line evicted first line from history.
 */
static void test_too_long_line_not_stored(void)
{
	uint8_t exp1_buf[HIST_BUF_SIZE];
	int max_len = get_max_buffer_len();

	init_test_buf(exp1_buf, sizeof(exp1_buf), 0);
	z_shell_history_init(&history);

	z_shell_history_put(&history, exp1_buf, max_len + 1);

	/*validate that nothing is stored */
	test_get(false, true, NULL, 0); /* empty */

	z_shell_history_put(&history, exp1_buf, 20);
	z_shell_history_put(&history, exp1_buf, max_len - 10);

	/* Test that long entry evicts older entry. */
	test_get(true, true, exp1_buf, max_len - 10);
	test_get(false, true, NULL, 0); /* only one entry */

	z_shell_history_purge(&history);
}

/* Test verifies that same line as the previous one is not stored in the
 * history.
 *
 * Test steps:
 * - initialize history.
 * - put same line twice.
 * - verify that only one line is in the history.
 */
static void test_no_duplicates_in_a_row(void)
{
	uint8_t exp1_buf[HIST_BUF_SIZE];

	init_test_buf(exp1_buf, sizeof(exp1_buf), 0);
	z_shell_history_init(&history);

	z_shell_history_put(&history, exp1_buf, 20);
	z_shell_history_put(&history, exp1_buf, 20);

	test_get(true, true, exp1_buf, 20);
	/* only one line stored. */
	test_get(false, true, NULL, 0);

	z_shell_history_purge(&history);
}

/* Test storing long lines in the history.
 *
 * * Test steps:
 * - initialize history.
 * - Put max length line 1 in history.
 * - Verify that it is present.
 * - Put max length line 2 in history.
 * - Verify that line 2 is present and line 1 was evicted.
 * - Put max length line 3 in history.
 * - Verify that line 3 is present and line 2 was evicted.
 */
static void test_storing_long_buffers(void)
{
	uint8_t exp1_buf[HIST_BUF_SIZE];
	uint8_t exp2_buf[HIST_BUF_SIZE];
	uint8_t exp3_buf[HIST_BUF_SIZE];
	int max_len = get_max_buffer_len();

	init_test_buf(exp1_buf, sizeof(exp1_buf), 0);
	init_test_buf(exp2_buf, sizeof(exp2_buf), 10);
	init_test_buf(exp3_buf, sizeof(exp3_buf), 20);

	z_shell_history_init(&history);

	z_shell_history_put(&history, exp1_buf, max_len);
	test_get(true, true, exp1_buf, max_len);
	test_get(false, true, NULL, 0); /* only one entry */

	z_shell_history_put(&history, exp2_buf, max_len);
	test_get(true, true, exp2_buf, max_len);
	test_get(false, true, NULL, 0); /* only one entry */

	z_shell_history_put(&history, exp3_buf, max_len);
	test_get(true, true, exp3_buf, max_len);
	test_get(false, true, NULL, 0); /* only one entry */

	z_shell_history_purge(&history);
}

void test_main(void)
{
	ztest_test_suite(shell_test_suite,
			ztest_unit_test(test_history_add_get),
			ztest_unit_test(test_history_purge),
			ztest_unit_test(test_history_get_up_and_down),
			ztest_unit_test(test_too_long_line_not_stored),
			ztest_unit_test(test_no_duplicates_in_a_row),
			ztest_unit_test(test_storing_long_buffers)
			);

	ztest_run_test_suite(shell_test_suite);
}
