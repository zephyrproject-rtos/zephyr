/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log list
 *
 */

#include <../subsys/logging/log_list.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>


void test_log_list(void)
{
	struct log_list_t my_list;

	log_list_init(&my_list);

	struct log_msg msg1, msg2;
	struct log_msg *msg;

	log_list_add_tail(&my_list, &msg1);

	msg = log_list_head_peek(&my_list);
	zassert_true(&msg1 == msg, "Unexpected head 0x%08X.\n", msg);

	msg = log_list_head_get(&my_list);

	zassert_true(&msg1 == msg, "Unexpected head 0x%08X.\n", msg);
	zassert_true(log_list_head_peek(&my_list) == NULL,
		     "Expected empty list.\n");

	/* two elements */
	log_list_add_tail(&my_list, &msg1);
	log_list_add_tail(&my_list, &msg2);

	msg = log_list_head_peek(&my_list);
	zassert_true(&msg1 == msg, "Unexpected head 0x%08X.\n", msg);

	msg = log_list_head_get(&my_list);
	zassert_true(&msg1 == msg, "Unexpected head 0x%08X.\n", msg);

	msg = log_list_head_peek(&my_list);
	zassert_true(&msg2 == msg, "Unexpected head 0x%08X.\n", msg);

	log_list_add_tail(&my_list, &msg1);

	msg = log_list_head_get(&my_list);
	zassert_true(&msg2 == msg, "Unexpected head 0x%08X.\n", msg);

	msg = log_list_head_get(&my_list);
	zassert_true(&msg1 == msg, "Unexpected head 0x%08X.\n", msg);

	msg = log_list_head_get(&my_list);
	zassert_true(msg == NULL, "Expected empty list.\n");
}

void test_log_list_multiple_items(void)
{
	struct log_list_t my_list;

	log_list_init(&my_list);

	struct log_msg msg[10];
	int i;

	for (i = 0; i < 10; i++) {
		log_list_add_tail(&my_list, &msg[i]);
	}

	for (i = 0; i < 10; i++) {
		zassert_true(&msg[i] == log_list_head_get(&my_list),
			     "Unexpected head.\n");
	}
	zassert_true(log_list_head_get(&my_list) == NULL,
		     "Expected empty list.\n");
}
/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_list,
			 ztest_unit_test(test_log_list),
			 ztest_unit_test(test_log_list_multiple_items));
	ztest_run_test_suite(test_log_list);
}
