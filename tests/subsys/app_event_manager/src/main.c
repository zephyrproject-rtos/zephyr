/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <app_event_manager/app_event_manager.h>

#include "sized_events.h"
#include "test_events.h"

static enum test_id cur_test_id;
static K_SEM_DEFINE(test_end_sem, 0, 1);
static bool expect_assert;

/* Provide custom assert post action handler to handle the assertion on OOM
 * error in Application Event Manager.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_ASSERT_NO_FILE_INFO));
void assert_post_action(const char *file, unsigned int line)
{
	printk("assert_post_action - file: %s (line: %u)\n", file, line);
	if (expect_assert) {
		expect_assert = false;
		printk("Assertion was expected.\n");
	} else {
		zassert(false, "", "Unexpected assertion reached.");
	}
}

void test_init(void)
{
	zassert_false(app_event_manager_init(), "Error when initializing");
}

static void test_start(enum test_id test_id)
{
	cur_test_id = test_id;
	struct test_start_event *ts = new_test_start_event();

	zassert_not_null(ts, "Failed to allocate event");
	ts->test_id = test_id;
	APP_EVENT_SUBMIT(ts);

	int err = k_sem_take(&test_end_sem, K_SECONDS(30));
	zassert_equal(err, 0, "Test execution hanged");
}

static void test_basic(void)
{
	test_start(TEST_BASIC);
}

static void test_data(void)
{
	test_start(TEST_DATA);
}

static void test_event_order(void)
{
	test_start(TEST_EVENT_ORDER);
}

static void test_subs_order(void)
{
	test_start(TEST_SUBSCRIBER_ORDER);
}

static void test_multicontext(void)
{
	test_start(TEST_MULTICONTEXT);
}

static void test_event_size_static(void)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_size1_event *ev_s1;
	struct test_size2_event *ev_s2;
	struct test_size3_event *ev_s3;
	struct test_size_big_event *ev_sb;

	ev_s1 = new_test_size1_event();
	zassert_equal(sizeof(*ev_s1), app_event_manager_event_size(&ev_s1->header),
		"Event size1 unexpected size");
	app_event_manager_free(ev_s1);

	ev_s2 = new_test_size2_event();
	zassert_equal(sizeof(*ev_s2), app_event_manager_event_size(&ev_s2->header),
		"Event size2 unexpected size");
	app_event_manager_free(ev_s2);

	ev_s3 = new_test_size3_event();
	zassert_equal(sizeof(*ev_s3), app_event_manager_event_size(&ev_s3->header),
		"Event size3 unexpected size");
	app_event_manager_free(ev_s3);

	ev_sb = new_test_size_big_event();
	zassert_equal(sizeof(*ev_sb), app_event_manager_event_size(&ev_sb->header),
		"Event size_big unexpected size");
	app_event_manager_free(ev_sb);
}

static void test_event_size_dynamic(void)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_dynamic_event *ev;

	ev = new_test_dynamic_event(0);
	zassert_equal(sizeof(*ev) + 0, app_event_manager_event_size(&ev->header),
		"Event dynamic with 0 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_event(10);
	zassert_equal(sizeof(*ev) + 10, app_event_manager_event_size(&ev->header),
		"Event dynamic with 10 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_event(100);
	zassert_equal(sizeof(*ev) + 100, app_event_manager_event_size(&ev->header),
		"Event dynamic with 100 elements unexpected size");
	app_event_manager_free(ev);
}

static void test_event_size_dynamic_with_data(void)
{
	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_dynamic_with_data_event *ev;

	ev = new_test_dynamic_with_data_event(0);
	zassert_equal(sizeof(*ev) + 0, app_event_manager_event_size(&ev->header),
		"Event dynamic with 0 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_with_data_event(10);
	zassert_equal(sizeof(*ev) + 10, app_event_manager_event_size(&ev->header),
		"Event dynamic with 10 elements unexpected size");
	app_event_manager_free(ev);

	ev = new_test_dynamic_with_data_event(100);
	zassert_equal(sizeof(*ev) + 100, app_event_manager_event_size(&ev->header),
		"Event dynamic with 100 elements unexpected size");
	app_event_manager_free(ev);
}

static void test_event_size_disabled(void)
{
	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROVIDE_EVENT_SIZE)) {
		ztest_test_skip();
		return;
	}

	struct test_size1_event *ev_s1;

	ev_s1 = new_test_size1_event();
	expect_assert = true;
	zassert_equal(0, app_event_manager_event_size(&ev_s1->header),
		"Event size1 unexpected size");
	zassert_false(expect_assert,
		"Assertion during app_event_manager_event_size function execution was expected");
	app_event_manager_free(ev_s1);
}

void test_oom_reset(void);

void test_main(void)
{
	ztest_test_suite(app_event_manager_tests,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_basic),
			 ztest_unit_test(test_data),
			 ztest_unit_test(test_event_order),
			 ztest_unit_test(test_subs_order),
			 ztest_unit_test(test_oom_reset),
			 ztest_unit_test(test_multicontext),
			 ztest_unit_test(test_event_size_static),
			 ztest_unit_test(test_event_size_dynamic),
			 ztest_unit_test(test_event_size_dynamic_with_data),
			 ztest_unit_test(test_event_size_disabled)
			 );

	ztest_run_test_suite(app_event_manager_tests);
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_test_end_event(aeh)) {
		struct test_end_event *ev = cast_test_end_event(aeh);

		zassert_equal(cur_test_id, ev->test_id,
			      "End test ID does not equal start test ID");
		cur_test_id = TEST_IDLE;
		k_sem_give(&test_end_sem);

		return false;
	}

	zassert_true(false, "Wrong event type received");
	return false;
}

APP_EVENT_LISTENER(test_main, app_event_handler);
APP_EVENT_SUBSCRIBE(test_main, test_end_event);
