/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */

#include <zephyr/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/ztest.h>

static const struct device *const controller = DEVICE_DT_GET(DT_NODELABEL(spi2));

ZTEST(mspi_esp32, test_idle_channel)
{
	zassert_true(device_is_ready(controller));
	/* Exercise the public API, which dispatches through get_channel_status. */
	zassert_ok(mspi_get_channel_status(controller, 0));
}

ZTEST(mspi_esp32, test_invalid_channel)
{
	zassert_equal(mspi_get_channel_status(controller, 1), -EINVAL);
}

ZTEST_SUITE(mspi_esp32, NULL, NULL, NULL, NULL, NULL);
