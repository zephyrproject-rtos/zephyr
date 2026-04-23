/*
 * Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include <zephyr/drivers/display.h>

static const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static struct display_capabilities disp_capabilities;
static enum display_event_result display_cb_handler(const struct device *dev, uint32_t evt,
	const struct display_event_data *data, void *user_data);
uint32_t cb_handle;
struct k_sem eventsem;

#if defined(CONFIG_STM32_LTDC)
/* dev->data is void *, use a typed wrapper to access the first LTDC field safely. */
struct display_callbacks_ltdc_data {
	LTDC_HandleTypeDef hltdc;
};

static inline LTDC_HandleTypeDef *display_callbacks_get_hltdc(const struct device *dev)
{
	struct display_callbacks_ltdc_data *data =
		(struct display_callbacks_ltdc_data *)dev->data;

	return &data->hltdc;
}
#endif /* CONFIG_STM32_LTDC */

static enum display_event_result display_cb_handler(const struct device *dev, uint32_t evt,
	const struct display_event_data *data, void *user_data)
{
	k_sem_give(&eventsem);

	return DISPLAY_EVENT_RESULT_HANDLED;
}

static void display_callbacks_before(void *text_fixture)
{
	display_blanking_off(dev);
	display_get_capabilities(dev, &disp_capabilities);

	if (cb_handle != 0U) {
		int res = display_unregister_event_cb(dev, cb_handle);

		zassert_ok(res, "Failed to unregister callback in test setup. Result: %i", res);
		cb_handle = 0U;
	}
	k_sem_init(&eventsem, 0, 1);
}

ZTEST(display_callbacks, test_line_event_registration)
{
	uint16_t target_line = disp_capabilities.y_resolution / 2;

#if defined(CONFIG_STM32_LTDC)
	int res = HAL_LTDC_ProgramLineEvent(display_callbacks_get_hltdc(dev), target_line);
#endif /* CONFIG_STM32_LTDC */

	res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_LINE_INT, true,
					      &cb_handle);

	zassert_ok(res, "Failed to register callback for line event. Result: %i", res);
	zassert_not_equal(cb_handle, 0U, "Registration failed, handle should be non-zero");

	zassert_ok(k_sem_take(&eventsem, K_MSEC(100)), "Callback was not called within timeout");
}

ZTEST(display_callbacks, test_vsync_registration)
{
	int res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_VSYNC,
		true, &cb_handle);

	zassert_ok(res, "Failed to register callback for vsync event. Result: %i", res);
	zassert_not_equal(cb_handle, 0U, "Registration failed, handle should be non-zero");

	zassert_ok(k_sem_take(&eventsem, K_MSEC(100)), "Callback was not called within timeout");
}

ZTEST(display_callbacks, test_null_dev_registration)
{
	ztest_set_assert_valid(true);
	display_register_event_cb(NULL, display_cb_handler, NULL, DISPLAY_EVENT_VSYNC, true,
				  &cb_handle);

	ztest_test_fail();
}

ZTEST(display_callbacks, test_null_cb_handler_registration)
{
	ztest_set_assert_valid(true);
	display_register_event_cb(dev, NULL, NULL, DISPLAY_EVENT_VSYNC, true,
				  &cb_handle);

	ztest_test_fail();
}

ZTEST_EXPECT_FAIL(display_callbacks, test_unreachable_linevent);
ZTEST(display_callbacks, test_unreachable_linevent)
{
	uint16_t target_line = disp_capabilities.y_resolution * 2;

#if defined(CONFIG_STM32_LTDC)
	int res = HAL_LTDC_ProgramLineEvent(display_callbacks_get_hltdc(dev), target_line);
#endif /* CONFIG_STM32_LTDC */

	res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_LINE_INT,
		true, &cb_handle);

	zassert_ok(res, "Failed to register callback for line event. Result: %i", res);
	zassert_not_equal(cb_handle, 0U, "Registration failed, handle should be non-zero");

	zassert_ok(k_sem_take(&eventsem, K_MSEC(100)),
		"Callback not called before timeout. Expected, line event is at %i, Y resolution is %i",
		target_line, disp_capabilities.y_resolution);
}

ZTEST_EXPECT_FAIL(display_callbacks, test_double_registration);
ZTEST(display_callbacks, test_double_registration)
{
	int res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_LINE_INT,
		true, &cb_handle);

	res = display_register_event_cb(dev, display_cb_handler, NULL, DISPLAY_EVENT_VSYNC, true,
		&cb_handle);

	zassert_ok(res, "Failed to register a second callback. Result: %i", res);
	zassert_not_equal(cb_handle, 0U,
		"Registration of second callback failed, handle should be non-zero");
}

ZTEST_SUITE(display_callbacks, NULL, NULL, display_callbacks_before, NULL, NULL);
