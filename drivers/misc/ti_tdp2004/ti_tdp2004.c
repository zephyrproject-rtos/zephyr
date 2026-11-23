/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tdp2004

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(ti_tdp2004, CONFIG_TI_TDP2004_LOG_LEVEL);

#define TI_TDP2004_TEST_MODE_REG 0x84
#define TI_TDP2004_TEST_MODE_DISABLE BIT(2)

struct ti_tdp2004_config {
	struct gpio_dt_spec redriver_pd_gpio;
	struct i2c_dt_spec redriver_dev;
};

static int ti_tdp2004_init(const struct device *dev)
{
	const struct ti_tdp2004_config *cfg = dev->config;
	const uint8_t redriver_tx_buf[] = {TI_TDP2004_TEST_MODE_REG, TI_TDP2004_TEST_MODE_DISABLE};
	int ret;

	if (!gpio_is_ready_dt(&cfg->redriver_pd_gpio)) {
		LOG_ERR("Redriver power GPIO not ready");
		return -ENODEV;
	}

	if (!i2c_is_ready_dt(&cfg->redriver_dev)) {
		LOG_ERR("Redriver I2C bus not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->redriver_pd_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure redriver power GPIO (%d)", ret);
		return ret;
	}

	ret = i2c_write_dt(&cfg->redriver_dev, redriver_tx_buf, sizeof(redriver_tx_buf));
	if (ret) {
		LOG_ERR("Failed to disable redriver test mode (%d)", ret);
	}

	return ret;
}

#define TI_TDP2004_INIT(n)						\
	static const struct ti_tdp2004_config ti_tdp2004_config##n = {	\
		.redriver_pd_gpio = GPIO_DT_SPEC_INST_GET(n, pd_gpios),	\
		.redriver_dev = I2C_DT_SPEC_INST_GET(n)};		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      &ti_tdp2004_init,				\
			      NULL,					\
			      NULL,					\
			      &ti_tdp2004_config##n,			\
			      POST_KERNEL,				\
			      CONFIG_TI_TDP2004_INIT_PRIORITY,		\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TI_TDP2004_INIT)
