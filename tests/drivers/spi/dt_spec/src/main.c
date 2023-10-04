/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test, CONFIG_LOG_DEFAULT_LEVEL);

ZTEST(spi_dt_spec, test_dt_spec)
{
	const struct spi_dt_spec spi_cs =
		SPI_DT_SPEC_GET(DT_NODELABEL(test_spi_dev_cs), 0, 0);

	LOG_DBG("spi_cs.bus = %p", spi_cs.bus);
	LOG_DBG("spi_cs.config.cs.gpio.port = %p", spi_cs.config.cs.gpio.port);
	LOG_DBG("spi_cs.config.cs.gpio.pin = %u", spi_cs.config.cs.gpio.pin);

	zassert_equal(spi_cs.bus, DEVICE_DT_GET(DT_NODELABEL(test_spi_cs)), "");
	zassert_true(spi_cs_is_gpio_dt(&spi_cs), "");
	zassert_equal(spi_cs.config.cs.gpio.port, DEVICE_DT_GET(DT_NODELABEL(test_gpio)), "");
	zassert_equal(spi_cs.config.cs.gpio.pin, 0x10, "");

	const struct spi_dt_spec spi_no_cs =
		SPI_DT_SPEC_GET(DT_NODELABEL(test_spi_dev_no_cs), 0, 0);

	LOG_DBG("spi_no_cs.bus = %p", spi_no_cs.bus);
	LOG_DBG("spi_no_cs.config.cs.gpio.port = %p", spi_no_cs.config.cs.gpio.port);

	zassert_equal(spi_no_cs.bus, DEVICE_DT_GET(DT_NODELABEL(test_spi_no_cs)), "");
	zassert_false(spi_cs_is_gpio_dt(&spi_no_cs), "");
}

ZTEST_SUITE(spi_dt_spec, NULL, NULL, NULL, NULL, NULL);
