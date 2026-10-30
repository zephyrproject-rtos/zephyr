/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/hid.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <sample_usbd.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/usb/class/usbh_hid.h>

#include "usbh_desc.h"
#include "usbh_device.h"
#include "hid_keyboard.h"

LOG_MODULE_REGISTER(usbh_test_hid, LOG_LEVEL_INF);

#define MAX_EVENTS 16

struct usbh_hid_keyboard_suite_fixture {
	size_t expected_events_index;
	struct input_event expected_events[MAX_EVENTS];
	size_t expected_usages_index;
	uint8_t expected_usages[MAX_EVENTS];
	struct k_sem input_sync;
	struct k_sem report_sync;
};

static struct device const *hid_dev = DEVICE_DT_GET(DT_NODELABEL(hid_keyboard));
static struct device const *usbh_hid_dev = DEVICE_DT_GET(DT_NODELABEL(any_hid_device));
static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

struct usbh_context *const uhs_ctx = &test_uhs_ctx;

static struct usbh_hid_keyboard_suite_fixture fixture = {};

static void *suite_setup(void)
{
	int result = 0;

	k_sem_init(&fixture.input_sync, 0, 1);
	k_sem_init(&fixture.report_sync, 0, 1);

	result = usbh_init(uhs_ctx);
	zassert_ok(result, "Failed to initialize USB host");

	result = usbh_enable(uhs_ctx);
	zassert_ok(result, "Failed to enable USB host");

	result = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(result, "Failed to signal bus reset");

	result = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(result, "Failed to signal bus resume");

	result = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(result, "Failed to enable SoF generator");

	result = hid_keyboard_register();
	zassert_ok(result, "Failed to register HID device");

	test_usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	result = usbd_init(test_usbd);
	zassert_ok(result, "Failed to initialize device support");

	result = usbd_enable(test_usbd);
	zassert_ok(result, "Failed to enable device support");

	/* Allow the host time to reset the device. */
	k_msleep(1000);

	result = usbh_hid_start_input_reports(usbh_hid_dev);
	zassert_ok(result, "Failed to start input reports");

	return NULL;
}

static void suite_shutdown(void *f)
{
	int result = 0;

	result = usbd_disable(test_usbd);
	zassert_ok(result, "Failed to disable device support");

	result = usbd_shutdown(test_usbd);
	zassert_ok(result, "Failed to shutdown device support");

	result = usbh_disable(uhs_ctx);
	zassert_ok(result, "Failed to disable USB host");

	result = usbh_shutdown(uhs_ctx);
	zassert_ok(result, "Failed to shutdown host support");
}

ZTEST_SUITE(usbh_hid_keyboard_suite, NULL, suite_setup, NULL, NULL, suite_shutdown);

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_protocol)
{
	int result = 0;
	uint8_t protocol_code = 0;

	result = usbh_hid_set_protocol(usbh_hid_dev, HID_PROTOCOL_BOOT);
	zassert_ok(result, "Unable to set protocol");

	result = usbh_hid_get_protocol(usbh_hid_dev, &protocol_code);
	zassert_ok(result, "Unable to get protocol");
	zassert_equal(protocol_code, HID_PROTOCOL_BOOT, "Wrong protocol");

	result = usbh_hid_set_protocol(usbh_hid_dev, HID_PROTOCOL_REPORT);
	zassert_ok(result, "Unable to set protocol");

	result = usbh_hid_get_protocol(usbh_hid_dev, &protocol_code);
	zassert_ok(result, "Unable to get protocol");
	zassert_equal(protocol_code, HID_PROTOCOL_REPORT, "Wrong protocol");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_output_report)
{
	uint8_t const report[] = {0xF};
	int result = 0;

	result = usbh_hid_set_report(usbh_hid_dev, HID_REPORT_TYPE_OUTPUT, 0, sizeof(report),
				     report);
	zassert_ok(result, "Unable to set report");

	zassert_equal(report[0], hid_keyboard_get_report_value(), "Wrong report value");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_idle)
{
	int result = 0;
	uint16_t idle_period_ms = 0;

	/* Idle period must be a multiple of 4 */
	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 501u);
	zassert_equal(result, -EINVAL, "Idle periods not divisible by 4 should be rejected");

	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 255u * 4u + 1u);
	zassert_equal(result, -EINVAL,
		      "Idle periods beyond what fits in a byte should be rejected");

	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 500u);
	zassert_ok(result, "Failed to set idle rate");

	result = usbh_hid_get_idle_rate(usbh_hid_dev, 0, &idle_period_ms);
	zassert_ok(result, "Failed to set idle rate");
	zassert_equal(idle_period_ms, 500u, "Wrong idle rate");

	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 0u);
	zassert_ok(result, "Failed to set idle rate");

	result = usbh_hid_get_idle_rate(usbh_hid_dev, 0, &idle_period_ms);
	zassert_ok(result, "Failed to set idle rate");
	zassert_equal(idle_period_ms, 0u, "Wrong idle rate");
}

int input_report_cb(struct usbh_hid_report_field const *field, uint8_t report_id,
		    size_t data_length, uint8_t const *data, size_t bit_index, void *user_data)
{
	size_t value_index = bit_index / 8;
	uint8_t const *value_ptr = &data[value_index];
	struct usbh_hid_keyboard_suite_fixture *fixture_ptr = user_data;

	zassert_equal(fixture_ptr, &fixture, "Invalid user data");

	if (usbh_hid_report_match_usage_page(field, HID_USAGE_GEN_KEYBOARD)) {
		/* Array data, each non-zero key is a usage id */
		if (USBH_HID_REPORT_DATA_IS_ARRAY(field->flags) && field->size == 8) {
			for (size_t key_index = 0; key_index < field->count; key_index++) {
				uint8_t usage_id = value_ptr[key_index];

				if (usage_id != 0) {
					uint16_t expected_usage =
						fixture_ptr->expected_usages
							[fixture_ptr->expected_usages_index];

					zassert_equal(usage_id, expected_usage,
						      "Invalid usage: expected 0x%X, found 0x%X",
						      expected_usage, usage_id);
					fixture_ptr->expected_usages_index++;

					if (fixture_ptr->expected_usages
						    [fixture_ptr->expected_usages_index] == 0) {
						k_sem_give(&fixture_ptr->report_sync);
					}
				}
			}
		} else if (USBH_HID_REPORT_DATA_IS_VARIABLE(field->flags) && field->size == 1u) {
			/* Variable data, a bitset where the position indicates the usage id */
			for (size_t key_position = 0; key_position < field->count; key_position++) {
				size_t key_index = key_position / 8;
				size_t key_shift = key_position % 8;
				uint16_t usage_id = usbh_hid_report_field_get_usage_id_by_index(
					field, key_position);

				if ((value_ptr[key_index] & (1 << key_shift)) > 0) {
					uint16_t expected_usage =
						fixture_ptr->expected_usages
							[fixture_ptr->expected_usages_index];

					zassert_equal(usage_id, expected_usage,
						      "Invalid usage: expected 0x%X, found 0x%X",
						      expected_usage, usage_id);
					fixture_ptr->expected_usages_index++;

					if (fixture_ptr->expected_usages
						    [fixture_ptr->expected_usages_index] == 0) {
						k_sem_give(&fixture_ptr->report_sync);
					}
				}
			}
		}
	}

	return 0;
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_input_single_keys)
{
	uint8_t control_usage = 0xE0;
	const uint8_t x_usage = 0x1B;
	int result = 0;

	result = usbh_hid_set_input_callback(usbh_hid_dev, input_report_cb, &fixture);
	zassert_ok(result);

	{
		/* X button press */
		uint8_t report[] = {0, 0, x_usage, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_X;
		fixture.expected_events[0].value = 1;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));
		fixture.expected_usages[0] = x_usage;

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);
		k_sem_take(&fixture.report_sync, K_FOREVER);
	}

	{
		/* X button release */
		uint8_t report[] = {0, 0, 0, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_X;
		fixture.expected_events[0].value = 0;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);

		/* No usages, no semaphore give */
		zassert_not_ok(k_sem_take(&fixture.report_sync, K_MSEC(100)));
	}

	{
		/* Left control modifier press */
		uint8_t report[] = {1, 0, 0, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_LEFTCTRL;
		fixture.expected_events[0].value = 1;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));
		fixture.expected_usages[0] = control_usage;

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);
		k_sem_take(&fixture.report_sync, K_FOREVER);
	}

	{
		/* Left control modifier release */
		uint8_t report[] = {0, 0, 0, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_LEFTCTRL;
		fixture.expected_events[0].value = 0;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);

		/* No usages, no semaphore give */
		zassert_not_ok(k_sem_take(&fixture.report_sync, K_MSEC(100)));
	}
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_input_multiple_keys)
{
	uint8_t control_usage = 0xE0;
	const uint8_t x_usage = 0x1B;
	const uint8_t y_usage = 0x1C;
	int result = 0;

	result = usbh_hid_set_input_callback(usbh_hid_dev, input_report_cb, &fixture);
	zassert_ok(result);

	{
		/* Left control, X and Y press */
		uint8_t report[] = {1, 0, x_usage, y_usage, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_LEFTCTRL;
		fixture.expected_events[0].value = 1;

		fixture.expected_events[1].type = INPUT_EV_KEY;
		fixture.expected_events[1].code = INPUT_KEY_X;
		fixture.expected_events[1].value = 1;

		fixture.expected_events[2].type = INPUT_EV_KEY;
		fixture.expected_events[2].code = INPUT_KEY_Y;
		fixture.expected_events[2].value = 1;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));
		fixture.expected_usages[0] = control_usage;
		fixture.expected_usages[1] = x_usage;
		fixture.expected_usages[2] = y_usage;

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);
		k_sem_take(&fixture.report_sync, K_FOREVER);
	}

	{
		/* X release */
		uint8_t report[] = {1, 0, 0x1C, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_LEFTCTRL;
		fixture.expected_events[0].value = 1;

		fixture.expected_events[1].type = INPUT_EV_KEY;
		fixture.expected_events[1].code = INPUT_KEY_Y;
		fixture.expected_events[1].value = 1;

		fixture.expected_events[2].type = INPUT_EV_KEY;
		fixture.expected_events[2].code = INPUT_KEY_X;
		fixture.expected_events[2].value = 0;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));
		fixture.expected_usages[0] = control_usage;
		fixture.expected_usages[1] = y_usage;

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);
		k_sem_take(&fixture.report_sync, K_FOREVER);
	}

	{
		/* Left control and Y release */
		uint8_t report[] = {0, 0, 0, 0, 0, 0, 0, 0};

		memset(fixture.expected_events, 0, sizeof(fixture.expected_events));
		fixture.expected_events_index = 0;
		fixture.expected_events[0].type = INPUT_EV_KEY;
		fixture.expected_events[0].code = INPUT_KEY_LEFTCTRL;
		fixture.expected_events[0].value = 0;

		fixture.expected_events[1].type = INPUT_EV_KEY;
		fixture.expected_events[1].code = INPUT_KEY_Y;
		fixture.expected_events[1].value = 0;

		fixture.expected_usages_index = 0;
		memset(fixture.expected_usages, 0, sizeof(fixture.expected_usages));

		hid_device_submit_report(hid_dev, sizeof(report), report);
		k_sem_take(&fixture.input_sync, K_FOREVER);

		/* No usages, no semaphore give */
		zassert_not_ok(k_sem_take(&fixture.report_sync, K_MSEC(100)));
	}
}

/*
 * The following tests exercise error paths of the usbh_hid public API by making the emulated
 * keyboard give an invalid or unsupported response to the host's request.
 */

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_get_report_descriptor_invalid_args)
{
	int result = usbh_hid_get_report_descriptor(usbh_hid_dev, NULL);

	zassert_equal(-EINVAL, result, "A NULL report pointer should be rejected");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_get_report)
{
	uint8_t buffer[8] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
	int result = 0;

	hid_keyboard_set_get_report_error(0);
	result =
		usbh_hid_get_report(usbh_hid_dev, HID_REPORT_TYPE_INPUT, 0, sizeof(buffer), buffer);
	zassert_ok(result, "Unable to get report");
	for (size_t i = 0; i < sizeof(buffer); i++) {
		zassert_equal(0, buffer[i], "Wrong report content");
	}

	/* The device rejects the request; this should surface as a stalled transfer */
	hid_keyboard_set_get_report_error(-EIO);
	result =
		usbh_hid_get_report(usbh_hid_dev, HID_REPORT_TYPE_INPUT, 0, sizeof(buffer), buffer);
	zassert_equal(-EPIPE, result, "A rejected Get Report should be surfaced as a stall");

	hid_keyboard_set_get_report_error(0);
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_idle_invalid_report_id)
{
	int result = 0;
	uint16_t idle_period_ms = 0;

	/* This device's report descriptor declares no report variants,
	 * so any ID but 0 is invalid.
	 */
	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0x7Fu, 100u);
	zassert_equal(-EINVAL, result, "Nonexistend report ID not rejected");

	result = usbh_hid_get_idle_rate(usbh_hid_dev, 0x7Fu, &idle_period_ms);
	zassert_equal(-EINVAL, result, "Nonexistend report ID not rejected");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_idle_not_supported)
{
	int result = 0;

	hid_keyboard_set_idle_supported(false);

	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 100u);
	zassert_equal(-EPIPE, result, "Unsupported SET_IDLE operation didn't stall");

	hid_keyboard_set_idle_supported(true);

	result = usbh_hid_set_idle_rate(usbh_hid_dev, 0, 0u);
	zassert_ok(result, "Failed to restore the idle rate");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_protocol_invalid_code)
{
	int result = 0;
	uint8_t protocol_code = 0;

	result = usbh_hid_set_protocol(usbh_hid_dev, HID_PROTOCOL_REPORT);
	zassert_ok(result, "Unable to set protocol");

	/* Only HID_PROTOCOL_BOOT (0) and HID_PROTOCOL_REPORT (1) are valid */
	result = usbh_hid_set_protocol(usbh_hid_dev, 2u);
	zassert_equal(-EPIPE, result, "An out-of-range protocol code should be rejected");

	/* The device should still be reporting the last protocol it actually accepted */
	result = usbh_hid_get_protocol(usbh_hid_dev, &protocol_code);
	zassert_ok(result, "Unable to get protocol");
	zassert_equal(protocol_code, HID_PROTOCOL_REPORT, "Wrong protocol");
}

ZTEST(usbh_hid_keyboard_suite, test_usbh_hid_keyboard_set_report_invalid_args)
{
	uint8_t const report[] = {0x00u};
	int result = 0;

	result = usbh_hid_set_report(usbh_hid_dev, HID_REPORT_TYPE_OUTPUT, 0, 0, NULL);
	zassert_equal(-EINVAL, result, "NULL report was accepted");

	result = usbh_hid_set_report(usbh_hid_dev, HID_REPORT_TYPE_OUTPUT, 0, 0, report);
	zassert_equal(-EINVAL, result, "Zero-length report was accepted");
}

static void verify_input_cb(struct input_event *evt, void *user_data)
{
	zassert_equal(fixture.expected_events[fixture.expected_events_index].type, evt->type,
		      "Wrong input event type");
	zassert_equal(fixture.expected_events[fixture.expected_events_index].code, evt->code,
		      "Wrong input event code");
	zassert_equal(fixture.expected_events[fixture.expected_events_index].value, evt->value,
		      "Wrong input event value");

	fixture.expected_events_index++;

	if (fixture.expected_events[fixture.expected_events_index].type == 0u) {
		k_sem_give(&fixture.input_sync);
	}
}

INPUT_CALLBACK_DEFINE(NULL, verify_input_cb, NULL);
