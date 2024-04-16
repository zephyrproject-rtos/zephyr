/*
 * Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_tcn75a

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tcn75a, CONFIG_SENSOR_LOG_LEVEL);

#include "tcn75a.h"

int tcn75a_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tcn75a_config *config = dev->config;
	struct tcn75a_data *data = dev->data;
	int ret;
	uint8_t temp_reg = TCN75A_TEMP_REG;
	uint8_t rx_buf[2];
	uint8_t adc_conf[2] = {TCN75A_CONFIG_REG, 0x0};
	/* This sensor only supports ambient temperature */
	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	if (config->oneshot_mode) {
		/* Oneshot mode, requires one shot bit to be set in config register */
		adc_conf[1] = TCN75A_CONFIG_ONEDOWN;
		ret = i2c_write_dt(&config->i2c_spec, adc_conf, 2);
		if (ret < 0) {
			return ret;
		}
	}

	/* Fetch a sample from the 2 byte ambient temperature register */
	ret = i2c_write_read_dt(&config->i2c_spec, &temp_reg, sizeof(temp_reg),
				rx_buf, sizeof(rx_buf));
	if (ret < 0) {
		return ret;
	}

	data->temp_sample = sys_get_be16(rx_buf);
	LOG_DBG("Raw sample: 0x%04x", data->temp_sample);

	return ret;
}

static int tcn75a_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tcn75a_data *data = dev->data;
	uint32_t temp_lsb;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Convert fixed point to sensor value  */
	val->val1 = data->temp_sample >> TCN75A_TEMP_MSB_POS;
	temp_lsb = (data->temp_sample & TCN75A_TEMP_LSB_MASK);
	val->val2 = TCN75A_FIXED_PT_TO_SENSOR(temp_lsb);
	return 0;
}

static const struct sensor_driver_api tcn75a_api = {
	.sample_fetch = &tcn75a_sample_fetch,
	.channel_get = &tcn75a_channel_get,
#ifdef CONFIG_TCN75A_TRIGGER
	.attr_get = &tcn75a_attr_get,
	.attr_set = &tcn75a_attr_set,
	.trigger_set = &tcn75a_trigger_set,
#endif
};

static int tcn75a_init(const struct device *dev)
{
	const struct tcn75a_config *config = dev->config;
	uint8_t adc_conf[2] = {TCN75A_CONFIG_REG, 0x0};

	if (!i2c_is_ready_dt(&config->i2c_spec)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	/* Set user selected resolution */
	adc_conf[1] |= TCN75A_CONFIG_RES(config->resolution);

	if (config->oneshot_mode) {
		if (adc_conf[1] != 0) {
			/* Oneshot mode only supports 9 bit resolution */
			LOG_ERR("Oneshot mode requires 9 bit resolution");
			return -ENODEV;
		}
		adc_conf[1] |= TCN75A_CONFIG_SHUTDOWN;
	}

#ifdef CONFIG_TCN75A_TRIGGER
	/* If user supplies an ALERT gpio, assume they want trigger support. */
	if (config->alert_gpios.port != NULL) {
		int ret;

		if (config->oneshot_mode) {
			LOG_ERR("Oneshot mode not supported with trigger");
			return -ENODEV;
		}

		ret = tcn75a_trigger_init(dev);
		if (ret < 0) {
			return ret;
		}
	}
#endif

	return i2c_write_dt(&config->i2c_spec, adc_conf, 2);
}

#ifdef CONFIG_TCN75A_TRIGGER
#define TCN75A_TRIGGER(n) .alert_gpios = GPIO_DT_SPEC_INST_GET_OR(n, alert_gpios, {}),
#else
#define TCN75A_TRIGGER(n)
#endif

#define TCN75A_INIT(n)                                                                             \
	static struct tcn75a_data tcn75a_data_##n;                                                 \
	static const struct tcn75a_config tcn75a_config_##n = {                                    \
		.i2c_spec = I2C_DT_SPEC_INST_GET(n),						   \
		.resolution = DT_INST_ENUM_IDX(n, resolution),					   \
		.oneshot_mode = DT_INST_PROP(n, oneshot_mode),					   \
		TCN75A_TRIGGER(n)								   \
	};											   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &tcn75a_init, NULL, &tcn75a_data_##n, &tcn75a_config_##n,  \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tcn75a_api);

DT_INST_FOREACH_STATUS_OKAY(TCN75A_INIT)
