/*
 * Copyright (c) 2025 Christoph Schnetzler
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/ztest.h>

static uint8_t modes[] = {
	MIPI_DBI_MODE_8080_BUS_8_BIT,
#ifndef MULTIPLE_INSTANCES
	MIPI_DBI_MODE_8080_BUS_9_BIT,
	MIPI_DBI_MODE_8080_BUS_16_BIT,
#endif
};

static const struct device *const mipi_dev = DEVICE_DT_GET(DT_NODELABEL(mipi_dbi));

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_command_write)
{
	int rslt;
	struct mipi_dbi_config config;

	uint8_t cmd = 0xff;
	uint8_t data[] = {0xff, 0x00, 0xff, 0x00};

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		config.mode = modes[i];
		rslt = mipi_dbi_command_write(mipi_dev, &config, cmd, data, sizeof(data));
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_write_display)
{
	int rslt;
	struct mipi_dbi_config config;
	struct display_buffer_descriptor descriptor;

	uint8_t data[] = {0xff, 0x00, 0xff, 0x00};

	descriptor.buf_size = sizeof(data);

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		config.mode = modes[i];
		rslt = mipi_dbi_write_display(mipi_dev, &config, NULL, &descriptor,
					      PIXEL_FORMAT_RGB_565);
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_reset)
{
	int rslt;

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		rslt = mipi_dbi_reset(mipi_dev, 100);
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}

#ifdef MULTIPLE_INSTANCES
static const struct device *const mipi_dev_1 = DEVICE_DT_GET(DT_NODELABEL(mipi_dbi_1));

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_multiple_instances_command_write)
{
	int rslt;
	struct mipi_dbi_config config;

	uint8_t cmd = 0xff;
	uint8_t data[] = {0xff, 0x00, 0xff, 0x00};

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		config.mode = modes[i];
		rslt = mipi_dbi_command_write(mipi_dev_1, &config, cmd, data, sizeof(data));
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_multiple_instances_write_display)
{
	int rslt;
	struct mipi_dbi_config config;
	struct display_buffer_descriptor descriptor;

	uint8_t data[] = {0xff, 0x00, 0xff, 0x00};

	descriptor.buf_size = sizeof(data);

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		config.mode = modes[i];
		rslt = mipi_dbi_write_display(mipi_dev_1, &config, NULL, &descriptor,
					      PIXEL_FORMAT_RGB_565);
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}

ZTEST(mipi_dbi_rpi_pico_pio, test_mipi_dbi_multiple_instances_reset)
{
	int rslt;

	for (int i = 0; i < ARRAY_SIZE(modes); ++i) {
		rslt = mipi_dbi_reset(mipi_dev_1, 100);
		zassert_equal(rslt, 0, "Expected 0 but was %u", rslt);
	}
}
#endif

static void *mipi_dbi_setup(void)
{
	__ASSERT_NO_MSG(device_is_ready(mipi_dev));

#ifdef MULTIPLE_INSTANCES
	__ASSERT_NO_MSG(device_is_ready(mipi_dev_1));
#endif

	return NULL;
}

ZTEST_SUITE(mipi_dbi_rpi_pico_pio, NULL, mipi_dbi_setup, NULL, NULL, NULL);
