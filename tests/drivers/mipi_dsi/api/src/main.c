/*
 * Copyright 2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dsi.h>
#include <zephyr/ztest.h>

/**
 * @addtogroup t_mipi_dsi_driver
 * @{
 * @defgroup t_mipi_dsi_api test_mipi_dsi_api
 * @}
 */

static const struct device *const mipi_dev = DEVICE_DT_GET(DT_ALIAS(mipi_dsi));

/**
 * Test the MIPI generic APIs to test read and write API functionality
 */
ZTEST(mipi_dsi_api, test_generic)
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
ZTEST(mipi_dsi_api, test_dcs)
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

static void *mipi_dsi_setup(void)
{
	__ASSERT_NO_MSG(device_is_ready(mipi_dev));

	return NULL;
}

ZTEST_SUITE(mipi_dsi_api, NULL, mipi_dsi_setup, NULL, NULL, NULL);
