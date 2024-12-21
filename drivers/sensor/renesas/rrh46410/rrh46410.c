/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rrh46410

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor/rrh46410.h>
#include "rrh46410.h"

LOG_MODULE_REGISTER(RRH46410, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t rrh46410_get_operation_mode(const struct device *dev)
{
	struct rrh46410_data *data = dev->data;
	int rc;
	uint8_t checksum = ~RRH46410_GET_OPERATION_MODE;
	uint8_t operation_mode;
	uint8_t get_operation[2];

	get_operation[0] = RRH46410_GET_OPERATION_MODE;
	get_operation[1] = checksum;
	rc = data->hw_tf->write_data(dev, get_operation, sizeof(get_operation));

	if (rc < 0) {
		LOG_ERR("Failed to send get.");
		return rc;
	}

	rc = data->hw_tf->read_data(dev, data->read_buffer, 2);
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	operation_mode = data->read_buffer[1];

	return operation_mode;
}

static int rrh46410_set_operation_mode(const struct device *dev)
{
	struct rrh46410_data *data = dev->data;
	int rc;
	uint8_t checksum = ~(RRH46410_SET_OPERATION_MODE + RRH46410_OPERATION_MODE_IAQ_2ND_GEN);
	uint8_t set_operation[3];

	set_operation[0] = RRH46410_SET_OPERATION_MODE;
	set_operation[1] = RRH46410_OPERATION_MODE_IAQ_2ND_GEN;
	set_operation[2] = checksum;

	rc = data->hw_tf->write_data(dev, set_operation, sizeof(set_operation));

	if (rc < 0) {
		LOG_ERR("Failed to send set.");
		return rc;
	}

	return 0;
}

static int rrh46410_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct rrh46410_data *data = dev->data;
	double val_rrh46410 = sensor_value_to_double(val);
	uint8_t encoded_humidity = (uint8_t) ((val_rrh46410 / 100) * 255);
	uint8_t command_data = encoded_humidity;
	uint8_t checksum = ~(RRH46410_SET_HUMIDITY + command_data);
	uint8_t set_humidity[3];
	int rc;

	set_humidity[0] = RRH46410_SET_HUMIDITY;
	set_humidity[1] = command_data;
	set_humidity[2] = checksum;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_RRH46410_HUMIDITY:
		rc = data->hw_tf->write_data(dev, set_humidity, sizeof(set_humidity));
		if (rc < 0) {
			LOG_ERR("Failed to send humidity.");
			return rc;
		}

		k_msleep(10);

		rc = data->hw_tf->read_data(dev, data->read_buffer, 1);
		if (rc < 0) {
			LOG_ERR("Failed to read data from device.");
			return rc;
		}

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rrh46410_read_sample(const struct device *dev, uint8_t *sample_counter,
				uint8_t *iaq_sample, uint16_t *tvoc_sample, uint16_t *etoh_sample,
				uint16_t *eco2_sample, uint8_t *reliaq_sample)
{
	struct rrh46410_data *data = dev->data;
	uint8_t status;
	int rc;

	rc = data->hw_tf->read_data(dev, data->read_buffer, sizeof(data->read_buffer));
	if (rc < 0) {
		LOG_ERR("Failed to read data from device.");
		return rc;
	}

	status = data->read_buffer[0];
	*sample_counter = data->read_buffer[1];
	*iaq_sample = data->read_buffer[2];
	*tvoc_sample = sys_get_be16(&data->read_buffer[3]);
	*etoh_sample = sys_get_be16(&data->read_buffer[5]);
	*eco2_sample = sys_get_be16(&data->read_buffer[7]);
	*reliaq_sample = sys_get_be16(&data->read_buffer[9]);

	if (status != 0x00) {
		LOG_ERR("Status error.");
	}

	return 0;
}

static int rrh46410_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct rrh46410_data *data = dev->data;
	int rc;
	uint8_t checksum = ~RRH46410_GET_MEASUREMENT_RESULTS;
	uint8_t fetch_sample[2];
	enum sensor_channel_rrh46410 rrh46410_chan = (enum sensor_channel_rrh46410)chan;

	if (chan != SENSOR_CHAN_ALL && rrh46410_chan != SENSOR_CHAN_IAQ &&
		rrh46410_chan != SENSOR_CHAN_TVOC && rrh46410_chan != SENSOR_CHAN_ETOH &&
		rrh46410_chan != SENSOR_CHAN_ECO2 && rrh46410_chan != SENSOR_CHAN_RELIAQ) {
		return -ENOTSUP;
	}

	fetch_sample[0] = RRH46410_GET_MEASUREMENT_RESULTS;
	fetch_sample[1] = checksum;

	rc = data->hw_tf->write_data(dev, fetch_sample, sizeof(fetch_sample));
	if (rc < 0) {
		LOG_ERR("Failed to send fetch.");
		return rc;
	}

	rc = rrh46410_read_sample(dev, &data->sample_counter, &data->iaq_sample, &data->tvoc_sample,
					&data->etoh_sample, &data->eco2_sample, &data->reliaq_sample);
	if (rc < 0) {
		LOG_ERR("Failed to fetch data.");
		return rc;
	}

	return 0;
}

static int rrh46410_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct rrh46410_data *data = dev->data;
	int32_t convert_val;

	switch ((enum sensor_channel_rrh46410)chan) {
	case SENSOR_CHAN_IAQ:
		convert_val = ((int32_t)data->iaq_sample) * 10;
		val->val1 = convert_val / 100;
		val->val2 = convert_val % 100;
		break;
	case SENSOR_CHAN_TVOC:
		convert_val = ((int32_t)data->tvoc_sample) * 100;
		val->val1 = convert_val / 1000000;
		val->val2 = convert_val % 1000000;
		break;
	case SENSOR_CHAN_ETOH:
		convert_val = ((int32_t)data->etoh_sample) * 100;
		val->val1 = convert_val / 10000;
		val->val2 = convert_val % 10000;
		break;
	case SENSOR_CHAN_ECO2:
		convert_val = (int32_t)data->eco2_sample;
		val->val1 = convert_val / 100;
		val->val2 = convert_val % 100;
		break;
	case SENSOR_CHAN_RELIAQ:
		convert_val = ((int32_t)data->reliaq_sample) / 10;
		val->val1 = convert_val * 10;
		val->val2 = convert_val % 10;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rrh46410_init(const struct device *dev)
{
	const struct rrh46410_config *cfg = dev->config;
	int err;
	int status;
	uint8_t rrh46410_mode;

	LOG_DBG("Initializing %s", dev->name);

	status = cfg->bus_init(dev);

	if (status < 0) {
		return status;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("The reset pin GPIO port is not ready.\n");
		return 0;
	}

	err = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Configuring GPIO pin failed: %d\n", err);
		return 0;
	}

	err = gpio_pin_set_dt(&cfg->reset_gpio, 1);
	if (err != 0) {
		LOG_ERR("Setting GPIO pin level failed: %d\n", err);
	}

	k_sleep(K_MSEC(100));

	err = gpio_pin_set_dt(&cfg->reset_gpio, 0);
	if (err != 0) {
		LOG_ERR("Setting GPIO pin level failed: %d\n", err);
	}

	k_sleep(K_MSEC(600));

	rrh46410_mode = rrh46410_get_operation_mode(dev);
	if (rrh46410_mode != RRH46410_OPERATION_MODE_IAQ_2ND_GEN) {
		rrh46410_set_operation_mode(dev);
	}

	return 0;
}

static const struct sensor_driver_api rrh46410_driver_api = {
								.sample_fetch = rrh46410_sample_fetch,
								.channel_get = rrh46410_channel_get,
								.attr_set = rrh46410_attr_set};

#define RRH46410_CONFIG_I2C(n)							 \
{														 \
	.bus_init = rrh46410_i2c_init,						 \
	.bus_cfg = { .i2c = I2C_DT_SPEC_INST_GET(n), },		 \
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios), \
}

#define  RRH46410_CONFIG_UART(n)								\
{																\
	.bus_init = rrh46410_uart_init,								\
	.bus_cfg = { .uart_dev = DEVICE_DT_GET(DT_INST_BUS(n)), },  \
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),		\
}

#define DEFINE_RRH46410(n)																		  \
	static struct rrh46410_data rrh46410_data_##n;												  \
                                                                                                  \
	static const struct rrh46410_config rrh46410_config_##n =									  \
		COND_CODE_1(DT_INST_ON_BUS(n, i2c),														  \
			    (RRH46410_CONFIG_I2C(n)),														  \
			    (RRH46410_CONFIG_UART(n)));														  \
                                                                                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(n, rrh46410_init, NULL, &rrh46410_data_##n, &rrh46410_config_##n,\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     				  \
				     &rrh46410_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_RRH46410)
