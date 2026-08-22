/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-FileCopyrightText: 2026 Abderrahmane JARMOUNI
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include <zephyr/drivers/display.h>

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(test_display_ctrl));
static struct display_capabilities disp_capabilities;
static enum display_event_result display_cb_handler(const struct device *dev, uint32_t evt,
	const struct display_event_data *data, void *user_data);
uint32_t cb_handle;
struct k_sem eventsem;

static enum display_event_result display_cb_handler(const struct device *dev, uint32_t evt,
	const struct display_event_data *data, void *user_data)
{
	k_sem_give(&eventsem);

	return DISPLAY_EVENT_RESULT_HANDLED;
}

static void display_callbacks_before(void *text_fixture)
{
	display_get_capabilities(dev, &disp_capabilities);
	k_sem_init(&eventsem, 0, 1);
}

ZTEST(display_callbacks, test_supported_events_registration)
{
	/* Test all supported events */
	uint32_t events[] = {
		DISPLAY_EVENT_LINE_INT,
		DISPLAY_EVENT_VSYNC,
		DISPLAY_EVENT_FRAME_DONE,
		DISPLAY_EVENT_FIFO_UNDERFLOW,
	};

	for (size_t i = 0; i < ARRAY_SIZE(events); i++) {
		uint32_t event = events[i];

		if (!(disp_capabilities.supported_events & event)) {
			continue;
		}

		/* Reset callback handle for this iteration */
		if (cb_handle != 0U) {
			display_unregister_event_cb(dev, cb_handle);
			cb_handle = 0U;
		}

		int res = display_register_event_cb(dev, display_cb_handler, NULL, event,
						    true, &cb_handle);

		if (res == -ENOTSUP) {
			/* ISR context not supported, try thread context */
			res = display_register_event_cb(dev, display_cb_handler, NULL, event,
							false, &cb_handle);
		}

		zassert_ok(res, "Failed to register callback for event %u. Result: %i", event, res);
		zassert_not_equal(cb_handle, 0U,
			"Registration failed for event %u, handle should be non-zero", event);

		zassert_ok(k_sem_take(&eventsem, K_MSEC(100)),
			"Callback was not called within timeout for event %u", event);
	}
}

ZTEST(display_callbacks, test_empty_event_mask)
{
	int res = display_register_event_cb(dev, display_cb_handler, NULL, 0U, true,
					    &cb_handle);

	zassert_equal(res, -EINVAL,
		"Empty event_mask should be rejected with -EINVAL. Got: %i", res);
}

ZTEST(display_callbacks, test_unsupported_event)
{
	/* Use an event bit that's unlikely to ever be defined */
	uint32_t unsupported_event = BIT(31);

	if (disp_capabilities.supported_events & unsupported_event) {
		ztest_test_skip();
	}

	int res = display_register_event_cb(dev, display_cb_handler, NULL, unsupported_event,
					    true, &cb_handle);

	zassert_equal(res, -ENOTSUP,
		"Unsupported event should be rejected with -ENOTSUP. Got: %i", res);
}

ZTEST_EXPECT_FAIL(display_callbacks, test_unreachable_linevent);
ZTEST(display_callbacks, test_unreachable_linevent)
{
	int res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_LINE_INT,
		true, &cb_handle);

	zassert_ok(res, "Failed to register callback for line event. Result: %i", res);
	zassert_not_equal(cb_handle, 0U, "Registration failed, handle should be non-zero");
}

ZTEST_EXPECT_FAIL(display_callbacks, test_double_registration);
ZTEST(display_callbacks, test_double_registration)
{
	uint32_t event = disp_capabilities.supported_events & -disp_capabilities.supported_events;
	/* Second registration should fail with -EBUSY */
	uint32_t cb_handle_2;

	/* First registration should succeed */
	int res = display_register_event_cb(dev, display_cb_handler, NULL, event, true,
					    &cb_handle);
	if (res == -ENOTSUP) {
		res = display_register_event_cb(dev, display_cb_handler, NULL, event, false,
						&cb_handle);
	}
	zassert_ok(res, "First callback registration failed. Result: %i", res);

	res = display_register_event_cb(dev, display_cb_handler, NULL, event, true,
					&cb_handle_2);
	zassert_equal(res, -EBUSY, "Second callback should be rejected with -EBUSY. Got: %i", res);
}

ZTEST_SUITE(display_callbacks, NULL, NULL, display_callbacks_before, NULL, NULL);
