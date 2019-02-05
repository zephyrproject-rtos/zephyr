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

#define MAX_BUF_SIZE 128
SHELL_HISTORY_DEFINE(history, MAX_BUF_SIZE, 8);

static void init_test_buf(u8_t *buf, size_t len, u8_t offset)
{
	for (int i = 0; i < len; i++) {
		buf[i] = offset + i;
	}
}

static void test_get(bool ok, bool up, u8_t *exp_buf, u16_t exp_len)
{
	bool res;
	u8_t out_buf[MAX_BUF_SIZE];
	u16_t out_len;

	out_len = sizeof(out_buf);

	res = shell_history_get(&history_history, up, out_buf, &out_len);

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
void test_history_add_get(void)
{
	u8_t exp_buf[MAX_BUF_SIZE];

	init_test_buf(exp_buf, sizeof(exp_buf), 0);

	shell_history_init(&history_history);

	test_get(false, true, NULL, 0);

	shell_history_put(&history_history, exp_buf, 20);

	test_get(true, true, exp_buf, 20);
}

/* Test verifies that after purging there is no line in the history. */
void test_history_purge(void)
{
	u8_t exp_buf[MAX_BUF_SIZE];

	init_test_buf(exp_buf, sizeof(exp_buf), 0);

	shell_history_init(&history_history);

	shell_history_put(&history_history, exp_buf, 20);
	shell_history_put(&history_history, exp_buf, 20);

	shell_history_purge(&history_history);

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
void test_history_get_up_and_down(void)
{
	u8_t exp1_buf[MAX_BUF_SIZE];
	u8_t exp2_buf[MAX_BUF_SIZE];
	u8_t exp3_buf[MAX_BUF_SIZE];

	init_test_buf(exp1_buf, sizeof(exp1_buf), 0);
	init_test_buf(exp2_buf, sizeof(exp2_buf), 10);
	init_test_buf(exp3_buf, sizeof(exp3_buf), 20);

	shell_history_init(&history_history);

	shell_history_put(&history_history, exp1_buf, 20);
	shell_history_put(&history_history, exp2_buf, 20);
	shell_history_put(&history_history, exp3_buf, 20);

	test_get(true, true, exp3_buf, 20); /* up - 3*/
	test_get(true, true, exp2_buf, 20); /* up - 2*/
	test_get(true, true, exp1_buf, 20); /* up - 1*/
	test_get(true, false, exp2_buf, 20); /* down - 2 */
	test_get(true, true, exp1_buf, 20);  /* up - 1*/
	test_get(true, false, exp2_buf, 20); /* down - 2 */
	test_get(true, false, exp3_buf, 20); /* down - 3 */
	test_get(false, false, NULL, 0); /* down - nothing */
}

void test_main(void)
{
	ztest_test_suite(shell_test_suite,
			ztest_unit_test(test_history_add_get),
			ztest_unit_test(test_history_purge),
			ztest_unit_test(test_history_get_up_and_down)
			);

	ztest_run_test_suite(shell_test_suite);
}
