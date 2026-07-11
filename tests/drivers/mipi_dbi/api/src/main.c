/*
 * Copyright (c) 2025 Christoph Schnetzler
 * Copyright (c) 2025 Hsiu-Chi Tsai
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/ztest.h>

/*
 * The API tests below drive a real MIPI DBI controller, so they are only built
 * where the "mipi_dbi" node exists (see the filter in testcase.yaml).
 */
#if DT_NODE_EXISTS(DT_NODELABEL(mipi_dbi))

static const uint8_t modes[] = {
	MIPI_DBI_MODE_8080_BUS_8_BIT,
#ifndef MULTIPLE_INSTANCES
	MIPI_DBI_MODE_8080_BUS_9_BIT,
	MIPI_DBI_MODE_8080_BUS_16_BIT,
#endif
};

static const struct device *const devices[] = {DEVICE_DT_GET(DT_NODELABEL(mipi_dbi)),
#ifdef MULTIPLE_INSTANCES
					       DEVICE_DT_GET(DT_NODELABEL(mipi_dbi_1))
#endif
};

ZTEST(mipi_dbi_api, test_mipi_dbi_command_write)
{
	int ret;
	struct mipi_dbi_config config;

	uint8_t cmd = 0xff;
	uint8_t data[] = {0x00, 0xff, 0x00, 0xff};

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		for (int j = 0; j < ARRAY_SIZE(modes); ++j) {
			config.mode = modes[j];
			ret = mipi_dbi_command_write(devices[i], &config, cmd, data, sizeof(data));
			zassert_equal(ret, 0, "Expected 0 but was %u", ret);
		}
	}
}

ZTEST(mipi_dbi_api, test_mipi_dbi_command_write_cmd_only)
{
	int ret;
	struct mipi_dbi_config config;

	uint8_t cmd = 0xff;

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		for (int j = 0; j < ARRAY_SIZE(modes); ++j) {
			config.mode = modes[j];
			ret = mipi_dbi_command_write(devices[i], &config, cmd, NULL, 0);
			zassert_equal(ret, 0, "Expected 0 but was %u", ret);
		}
	}
}

ZTEST(mipi_dbi_api, test_mipi_dbi_write_display)
{
	int ret;
	struct mipi_dbi_config config;
	struct display_buffer_descriptor descriptor;

	uint8_t data[] = {0x00, 0xff, 0x00, 0xff};

	descriptor.buf_size = sizeof(data);

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		for (int j = 0; j < ARRAY_SIZE(modes); ++j) {
			config.mode = modes[j];
			ret = mipi_dbi_write_display(devices[i], &config, data, &descriptor,
						      PIXEL_FORMAT_RGB_565);
			zassert_equal(ret, 0, "Expected 0 but was %u", ret);
		}
	}
}

ZTEST(mipi_dbi_api, test_mipi_dbi_reset)
{
	int ret;

	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		for (int j = 0; j < ARRAY_SIZE(modes); ++j) {
			ret = mipi_dbi_reset(devices[i], 100);
			zassert_equal(ret, 0, "Expected 0 but was %u", ret);
		}
	}
}

static void *mipi_dbi_setup(void)
{
	for (int i = 0; i < ARRAY_SIZE(devices); ++i) {
		__ASSERT_NO_MSG(device_is_ready(devices[i]));
	}

	return NULL;
}

ZTEST_SUITE(mipi_dbi_api, NULL, mipi_dbi_setup, NULL, NULL, NULL);

#endif /* DT_NODE_EXISTS(DT_NODELABEL(mipi_dbi)) */

#if DT_NODE_EXISTS(DT_NODELABEL(testdev))

/*
 * MIPI_DBI_CONFIG_DT() must populate struct mipi_dbi_config.color_coding from
 * the optional "color-coding" devicetree property, and fall back to RGB565
 * when the property is omitted. Uses stub controllers, so it runs on native_sim.
 */
ZTEST(mipi_dbi_config, test_color_coding_from_dt)
{
	struct mipi_dbi_config config = MIPI_DBI_CONFIG_DT(DT_NODELABEL(testdev), 0, 0);

	zassert_equal(config.color_coding, MIPI_DBI_MODE_RGB888_1,
		      "Expected RGB888_1 (0x%x) but was 0x%x", MIPI_DBI_MODE_RGB888_1,
		      config.color_coding);
}

ZTEST(mipi_dbi_config, test_color_coding_default)
{
	struct mipi_dbi_config config =
		MIPI_DBI_CONFIG_DT(DT_NODELABEL(testdev_default), 0, 0);

	zassert_equal(config.color_coding, MIPI_DBI_MODE_RGB565,
		      "Expected default RGB565 (0x%x) but was 0x%x", MIPI_DBI_MODE_RGB565,
		      config.color_coding);
}

ZTEST_SUITE(mipi_dbi_config, NULL, NULL, NULL, NULL, NULL);

#endif /* DT_NODE_EXISTS(DT_NODELABEL(testdev)) */
