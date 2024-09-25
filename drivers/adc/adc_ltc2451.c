/* LLTC LTC2451 ADC
 *
 * Copyright (c) 2023 Brill Power Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(ltc2451, CONFIG_ADC_LOG_LEVEL);

#define DT_DRV_COMPAT lltc_ltc2451

struct ltc2451_config {
	struct i2c_dt_spec i2c;
	uint8_t conversion_speed;
};

static int ltc2451_channel_setup(const struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("Invalid channel id '%d'", channel_cfg->channel_id);
		return -EINVAL;
	}

	return 0;
}

static int ltc2451_set_conversion_speed(const struct device *dev, uint8_t conversion_speed)
{
	const struct ltc2451_config *config = dev->config;
	uint8_t wr_buf[1];
	int err;

	if (conversion_speed == 60) {
		wr_buf[0] = 0;
	} else if (conversion_speed == 30) {
		wr_buf[0] = 1;
	} else {
		LOG_ERR("Invalid conversion speed selected");
		return -EINVAL;
	}

	err = i2c_write_dt(&config->i2c, wr_buf, sizeof(wr_buf));

	if (err != 0) {
		LOG_ERR("LTC write failed (err %d)", err);
	}

	return err;
}

static int ltc2451_read_latest_conversion(const struct device *dev,
					  const struct adc_sequence *sequence)
{
	const struct ltc2451_config *config = dev->config;
	uint8_t rd_buf[2];
	uint16_t *value_buf;
	int err = i2c_read_dt(&config->i2c, rd_buf, sizeof(rd_buf));

	if (err == 0) {
		value_buf = (uint16_t *)sequence->buffer;
		value_buf[0] = sys_get_be16(rd_buf);
	} else {
		LOG_ERR("LTC read failed (err %d)", err);
	}

	return err;
}

static int ltc2451_init(const struct device *dev)
{
	const struct ltc2451_config *config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	return ltc2451_set_conversion_speed(dev, config->conversion_speed);
}

static const struct adc_driver_api ltc2451_api = {
	.channel_setup = ltc2451_channel_setup,
	.read = ltc2451_read_latest_conversion,
};

#define LTC2451_DEFINE(index) \
	static const struct ltc2451_config ltc2451_cfg_##index = { \
		.i2c = I2C_DT_SPEC_INST_GET(index), \
		.conversion_speed = DT_INST_PROP(index, conversion_speed), \
	}; \
 \
	DEVICE_DT_INST_DEFINE(index, &ltc2451_init, NULL, NULL, \
			      &ltc2451_cfg_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			      &ltc2451_api);

DT_INST_FOREACH_STATUS_OKAY(LTC2451_DEFINE)
