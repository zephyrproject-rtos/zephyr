/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dsi.h>
#include <ztest.h>

/**
 * @addtogroup t_mipi_dsi_driver
 * @{
 * @defgroup t_mipi_dsi_api test_mipi_dsi_api
 * @}
 */

static const struct device *mipi_dev = DEVICE_DT_GET(DT_ALIAS(mipi_dsi));

/**
 * Test the MIPI generic APIs to test read and write API functionality
 */
static void test_generic(void)
{
	uint8_t rx_buf[2];
	uint8_t param[2];
	ssize_t ret;

	param[0] = MIPI_DCS_SET_DISPLAY_ON;
	ret = mipi_dsi_generic_write(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL, param, 1);
	zassert(ret >= 0, "Failed to write", NULL);

	param[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	param[1] = 200;
	ret = mipi_dsi_generic_write(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL, param, 2);
	zassert(ret >= 0, "Failed to write", NULL);

	memset(rx_buf, 0, sizeof(rx_buf));

	param[0] = MIPI_DCS_GET_DISPLAY_BRIGHTNESS;
	ret = mipi_dsi_generic_read(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL, param, 1, rx_buf, 2);

	if (ret != -ENOTSUP) {
		zassert(ret >= 0, "Failed to do a generic read", NULL);
	}

}

/**
 * Test the MIPI DCS APIs to test read and write API functionality
 */
static void test_dcs(void)
{
	uint8_t rx_buf[2];
	uint8_t param[2];
	ssize_t ret;

	ret = mipi_dsi_dcs_write(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL,
				 MIPI_DCS_SET_DISPLAY_ON, NULL, 0);
	zassert(ret >= 0, "Failed to write", NULL);

	param[0] = 200;
	ret = mipi_dsi_dcs_write(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL,
				 MIPI_DCS_SET_DISPLAY_BRIGHTNESS, param, 1);
	zassert(ret >= 0, "Failed to write", NULL);

	memset(rx_buf, 0, sizeof(rx_buf));

	param[0] = MIPI_DCS_GET_DISPLAY_BRIGHTNESS;
	ret = mipi_dsi_dcs_read(mipi_dev, CONFIG_MIPI_DSI_TEST_CHANNEL,
				MIPI_DCS_GET_DISPLAY_BRIGHTNESS, rx_buf, 2);

	if (ret != -ENOTSUP) {
		zassert(ret >= 0, "Failed to do a dcs read", NULL);
	}
}

void test_main(void)
{
	__ASSERT_NO_MSG(device_is_ready(mipi_dev));

	ztest_test_suite(mipi_dsi_api_tests,
			ztest_unit_test(test_generic),
			ztest_unit_test(test_dcs)
			);

	ztest_run_test_suite(mipi_dsi_api_tests);
}
