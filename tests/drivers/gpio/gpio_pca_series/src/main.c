/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/ztest.h>

#include "pca9555_emul.h"

#define PCA9555_REG_INPUT_PORT0              0x00
#define PCA9555_REG_OUTPUT_PORT0             0x02
#define PCA9555_REG_POLARITY_INVERSION_PORT0 0x04
#define PCA9555_REG_CONFIGURATION_PORT0      0x06

static const struct device *const auto_reset_dev = DEVICE_DT_GET(DT_NODELABEL(pca9555_auto_reset));
static const struct emul *const auto_reset_emul = EMUL_DT_GET(DT_NODELABEL(pca9555_auto_reset));
static const struct device *const no_auto_reset_dev =
	DEVICE_DT_GET(DT_NODELABEL(pca9555_no_auto_reset));
static const struct emul *const no_auto_reset_emul =
	EMUL_DT_GET(DT_NODELABEL(pca9555_no_auto_reset));

ZTEST(gpio_pca_series, test_automatic_reset_restores_all_basic_registers)
{
	zassert_true(device_is_ready(auto_reset_dev));

	zexpect_equal(pca9555_emul_get_word(auto_reset_emul, PCA9555_REG_OUTPUT_PORT0), UINT16_MAX);
	zexpect_equal(pca9555_emul_get_word(auto_reset_emul, PCA9555_REG_POLARITY_INVERSION_PORT0),
		      0);
	zexpect_equal(pca9555_emul_get_word(auto_reset_emul, PCA9555_REG_CONFIGURATION_PORT0),
		      UINT16_MAX);
}

ZTEST(gpio_pca_series, test_no_auto_reset_preserves_polarity_inversion)
{
	zassert_true(device_is_ready(no_auto_reset_dev));
	zexpect_equal(
		pca9555_emul_get_word(no_auto_reset_emul, PCA9555_REG_POLARITY_INVERSION_PORT0),
		PCA9555_EMUL_INITIAL_POLARITY);
}

ZTEST(gpio_pca_series, test_port_get_returns_both_input_ports)
{
	gpio_port_value_t value = 0;

	pca9555_emul_set_word(auto_reset_emul, PCA9555_REG_INPUT_PORT0, 0xa55a);

	zassert_ok(gpio_port_get_raw(auto_reset_dev, &value));
	zexpect_equal(value, 0xa55a);
}

ZTEST_SUITE(gpio_pca_series, NULL, NULL, NULL, NULL, NULL);
