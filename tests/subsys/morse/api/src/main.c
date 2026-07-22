/*
 * Copyright (c) 2026 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/morse/morse.h>

static const struct device *const morse_dev =
	DEVICE_DT_GET(DT_NODELABEL(morse));

static void tx_callback(void *ctx, int status)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(status);
}

static void rx_callback(void *ctx, enum morse_rx_state state,
			uint32_t data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(state);
	ARG_UNUSED(data);
}

ZTEST(morse_api, test_device_ready)
{
	zassert_true(device_is_ready(morse_dev),
		     "Morse device not ready");
}

ZTEST(morse_api, test_send_null_dev)
{
	uint8_t data[] = "E";

	zassert_equal(morse_send(NULL, data, sizeof(data) - 1),
		      -EINVAL);
}

ZTEST(morse_api, test_send_null_data_nonzero_size)
{
	zassert_equal(morse_send(morse_dev, NULL, 5), -EINVAL);
}

ZTEST(morse_api, test_send_query_idle)
{
	zassert_equal(morse_send(morse_dev, NULL, 0), 0,
		      "Device should report idle");
}

ZTEST(morse_api, test_manage_callbacks_null_dev)
{
	zassert_equal(
		morse_manage_callbacks(NULL, tx_callback,
				       rx_callback, NULL),
		-EINVAL);
}

ZTEST(morse_api, test_manage_callbacks_register)
{
	int ret;

	ret = morse_manage_callbacks(morse_dev, tx_callback,
				     rx_callback, NULL);
	zassert_equal(ret, 0, "Register callbacks failed: %d", ret);
}

ZTEST(morse_api, test_manage_callbacks_unregister)
{
	int ret;

	ret = morse_manage_callbacks(morse_dev, NULL, NULL, NULL);
	zassert_equal(ret, 0,
		      "Unregister callbacks failed: %d", ret);
}

ZTEST(morse_api, test_set_config_valid)
{
	int ret;

	ret = morse_set_config(morse_dev, 10);
	zassert_equal(ret, 0, "Config 10 WPM failed: %d", ret);

	ret = morse_set_config(morse_dev, 30);
	zassert_equal(ret, 0, "Config 30 WPM failed: %d", ret);

	/* Restore default */
	ret = morse_set_config(morse_dev, 20);
	zassert_equal(ret, 0, "Config 20 WPM failed: %d", ret);
}

ZTEST(morse_api, test_set_config_zero_speed)
{
	zassert_equal(morse_set_config(morse_dev, 0), -EINVAL,
		      "Zero speed should return -EINVAL");
}

/*
 * NOTE: This test must run last. morse_send with an invalid
 * character sets data_size before morse_load() fails, leaving
 * the device in a pseudo-busy state that cannot be cleared
 * through the public API. The "zz_" prefix ensures alphabetical
 * ordering places it after all other tests.
 */
ZTEST(morse_api, test_zz_send_invalid_char)
{
	uint8_t data[] = {0x01};

	zassert_equal(morse_send(morse_dev, data, 1), -ENOENT,
		      "Invalid char should return -ENOENT");
}

ZTEST_SUITE(morse_api, NULL, NULL, NULL, NULL, NULL);
