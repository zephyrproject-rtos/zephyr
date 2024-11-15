/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_jc_42_4_temp

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "jc42.h"

LOG_MODULE_REGISTER(JC42, CONFIG_SENSOR_LOG_LEVEL);

int jc42_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct jc42_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), val, sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

int jc42_reg_write_16bit(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct jc42_config *cfg = dev->config;

	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

int jc42_reg_write_8bit(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct jc42_config *cfg = dev->config;
	uint8_t buf[2] = {
		reg,
		val,
	};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int jc42_set_temperature_resolution(const struct device *dev, uint8_t resolution)
{
	return jc42_reg_write_8bit(dev, JC42_REG_RESOLUTION, resolution);
}

static int jc42_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct jc42_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	return jc42_reg_read(dev, JC42_REG_TEMP_AMB, &data->reg_val);
}

static int jc42_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	const struct jc42_data *data = dev->data;
	int temp = jc42_temp_signed_from_reg(data->reg_val);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = temp / JC42_TEMP_SCALE_CEL;
	temp -= val->val1 * JC42_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / JC42_TEMP_SCALE_CEL;

	return 0;
}

static const struct sensor_driver_api jc42_api_funcs = {
	.sample_fetch = jc42_sample_fetch,
	.channel_get = jc42_channel_get,
#ifdef CONFIG_JC42_TRIGGER
	.attr_set = jc42_attr_set,
	.trigger_set = jc42_trigger_set,
#endif /* CONFIG_JC42_TRIGGER */
};

int jc42_init(const struct device *dev)
{
	const struct jc42_config *cfg = dev->config;
	int rc = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	rc = jc42_set_temperature_resolution(dev, cfg->resolution);
	if (rc) {
		LOG_ERR("Could not set the resolution of jc42 module");
		return rc;
	}

#ifdef CONFIG_JC42_TRIGGER
	if (cfg->int_gpio.port) {
		rc = jc42_setup_interrupt(dev);
	}
#endif /* CONFIG_JC42_TRIGGER */

	return rc;
}

#define JC42_DEFINE(inst)                                                                          \
	static struct jc42_data jc42_data_##inst;                                                  \
                                                                                                   \
	static const struct jc42_config jc42_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = DT_INST_PROP(inst, resolution),                                      \
		IF_ENABLED(CONFIG_JC42_TRIGGER,                                                    \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),))};        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, jc42_init, NULL, &jc42_data_##inst,                     \
				     &jc42_config_##inst, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &jc42_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(JC42_DEFINE)
