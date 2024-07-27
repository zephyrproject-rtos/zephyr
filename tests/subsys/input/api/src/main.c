/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/input/input.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>

static const struct device fake_dev;
static int message_count_filtered;
static int message_count_unfiltered;

#if CONFIG_INPUT_MODE_THREAD

static K_SEM_DEFINE(cb_start, 1, 1);
static K_SEM_DEFINE(cb_done, 1, 1);

static void input_cb_filtered(struct input_event *evt, void *user_data)
{
	TC_PRINT("%s: %d\n", __func__, message_count_filtered);

	k_sem_take(&cb_start, K_FOREVER);

	if (evt->dev == &fake_dev && evt->code == message_count_filtered) {
		message_count_filtered++;
	}

	k_sem_give(&cb_start);
}
INPUT_CALLBACK_DEFINE(&fake_dev, input_cb_filtered, NULL);

static void input_cb_unfiltered(struct input_event *evt, void *user_data)
{
	TC_PRINT("%s: %d\n", __func__, message_count_unfiltered);

	message_count_unfiltered++;

	if (message_count_unfiltered == CONFIG_INPUT_QUEUE_MAX_MSGS + 1) {
		TC_PRINT("cb: done\n");
		k_sem_give(&cb_done);
	}
}
INPUT_CALLBACK_DEFINE(NULL, input_cb_unfiltered, NULL);

ZTEST(input_api, test_sequence_thread)
{
	int i;
	int ret;

	message_count_filtered = 0;
	message_count_unfiltered = 0;

	k_sem_take(&cb_start, K_FOREVER);
	k_sem_take(&cb_done, K_FOREVER);

	/* fill the queue */
	for (i = 0; i < CONFIG_INPUT_QUEUE_MAX_MSGS; i++) {
		TC_PRINT("report: %d\n", i);
		ret = input_report_key(&fake_dev, i, 1, false, K_FOREVER);
		zassert_equal(ret, 0, "ret: %d", ret);
	}

	/* one extra with no dev to acconut for the message pending in the
	 * locked cb
	 */
	ret = input_report_key(NULL, 0, 1, false, K_FOREVER);
	zassert_equal(ret, 0, "ret: %d", ret);

	zassert_false(input_queue_empty());

	/* next message finds the queue full */
	ret = input_report_key(&fake_dev, 0, 1, false, K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, "ret: %d", ret);

	k_sem_give(&cb_start);

	/* wait for cb to get all the messages */
	k_sem_take(&cb_done, K_FOREVER);

	zassert_equal(message_count_filtered, CONFIG_INPUT_QUEUE_MAX_MSGS);
	zassert_equal(message_count_unfiltered, CONFIG_INPUT_QUEUE_MAX_MSGS + 1);
}

#else /* CONFIG_INPUT_MODE_THREAD */

static void input_cb_filtered(struct input_event *evt, void *user_data)
{
	if (evt->dev == &fake_dev) {
		message_count_filtered++;
	}
}
INPUT_CALLBACK_DEFINE(&fake_dev, input_cb_filtered, NULL);

static void input_cb_unfiltered(struct input_event *evt, void *user_data)
{
	message_count_unfiltered++;
}
INPUT_CALLBACK_DEFINE(NULL, input_cb_unfiltered, NULL);

ZTEST(input_api, test_synchronous)
{
	int ret;

	message_count_filtered = 0;
	message_count_unfiltered = 0;

	ret = input_report_key(&fake_dev, 0, 1, false, K_FOREVER);
	zassert_equal(ret, 0, "ret: %d", ret);

	ret = input_report_key(NULL, 0, 1, false, K_FOREVER);
	zassert_equal(ret, 0, "ret: %d", ret);

	zassert_equal(message_count_filtered, 1);
	zassert_equal(message_count_unfiltered, 2);
}

static struct input_event last_event;

static void input_cb_last_event(struct input_event *evt, void *user_data)
{
	memcpy(&last_event, evt, sizeof(last_event));
}
INPUT_CALLBACK_DEFINE(NULL, input_cb_last_event, NULL);

ZTEST(input_api, test_report_apis)
{
	int ret;

	ret = input_report_key(&fake_dev, INPUT_KEY_A, 1, false, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_KEY);
	zassert_equal(last_event.code, INPUT_KEY_A);
	zassert_equal(last_event.value, 1);
	zassert_equal(last_event.sync, 0);

	ret = input_report_key(&fake_dev, INPUT_KEY_B, 1234, true, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_KEY);
	zassert_equal(last_event.code, INPUT_KEY_B);
	zassert_equal(last_event.value, 1); /* key events are always 0 or 1 */
	zassert_equal(last_event.sync, 1);

	ret = input_report_abs(&fake_dev, INPUT_ABS_X, 100, false, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_ABS);
	zassert_equal(last_event.code, INPUT_ABS_X);
	zassert_equal(last_event.value, 100);
	zassert_equal(last_event.sync, 0);

	ret = input_report_rel(&fake_dev, INPUT_REL_Y, -100, true, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_REL);
	zassert_equal(last_event.code, INPUT_REL_Y);
	zassert_equal(last_event.value, -100);
	zassert_equal(last_event.sync, 1);

	ret = input_report(&fake_dev, INPUT_EV_MSC, INPUT_MSC_SCAN, 0x12341234, true, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_MSC);
	zassert_equal(last_event.code, INPUT_MSC_SCAN);
	zassert_equal(last_event.value, 0x12341234);
	zassert_equal(last_event.sync, 1);

	ret = input_report(&fake_dev, INPUT_EV_VENDOR_START, 0xaaaa, 0xaaaaaaaa, true, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_VENDOR_START);
	zassert_equal(last_event.code, 0xaaaa);
	zassert_equal(last_event.value, 0xaaaaaaaa);
	zassert_equal(last_event.sync, 1);

	ret = input_report(&fake_dev, INPUT_EV_VENDOR_STOP, 0x5555, 0x55555555, true, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(last_event.dev, &fake_dev);
	zassert_equal(last_event.type, INPUT_EV_VENDOR_STOP);
	zassert_equal(last_event.code, 0x5555);
	zassert_equal(last_event.value, 0x55555555);
	zassert_equal(last_event.sync, 1);
}

#endif /* CONFIG_INPUT_MODE_THREAD */

ZTEST_SUITE(input_api, NULL, NULL, NULL, NULL, NULL);
