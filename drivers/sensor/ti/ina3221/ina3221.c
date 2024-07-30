/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina3221

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include "ina3221.h"

LOG_MODULE_REGISTER(INA3221, CONFIG_SENSOR_LOG_LEVEL);

#define MAX_RETRIES 10

static int reg_read(const struct device *dev, uint8_t reg_addr, uint16_t *reg_data)
{
	const struct ina3221_config *cfg = dev->config;
	uint8_t rx_buf[2];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), rx_buf, sizeof(rx_buf));

	*reg_data = sys_get_be16(rx_buf);

	return rc;
}

static int reg_write(const struct device *dev, uint8_t addr, uint16_t reg_data)
{
	const struct ina3221_config *cfg = dev->config;
	uint8_t tx_buf[3];

	tx_buf[0] = addr;
	sys_put_be16(reg_data, &tx_buf[1]);

	return i2c_write_dt(&cfg->bus, tx_buf, sizeof(tx_buf));
}

static int ina3221_init(const struct device *dev)
{
	int ret;
	const struct ina3221_config *cfg = dev->config;
	struct ina3221_data *data = dev->data;
	uint16_t reg_data;

	__ASSERT_NO_MSG(cfg->avg_mode < 8);
	__ASSERT_NO_MSG(cfg->conv_time_bus < 8);
	__ASSERT_NO_MSG(cfg->conv_time_shunt < 8);

	/* select first enabled channel */
	for (size_t i = 0; i < 3; ++i) {
		if (cfg->enable_channel[i]) {
			data->selected_channel = i;
			break;
		}
	}

	/* check bus */
	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("Device not ready.");
		return -ENODEV;
	}

	/* check connection */
	ret = reg_read(dev, INA3221_MANUF_ID, &reg_data);
	if (ret) {
		LOG_ERR("No answer from sensor.");
		return ret;
	}
	if (reg_data != INA3221_MANUF_ID_VALUE) {
		LOG_ERR("Unexpected manufacturer ID: 0x%04x", reg_data);
		return -EFAULT;
	}
	ret = reg_read(dev, INA3221_CHIP_ID, &reg_data);
	if (ret) {
		return ret;
	}
	if (reg_data != INA3221_CHIP_ID_VALUE) {
		LOG_ERR("Unexpected chip ID: 0x%04x", reg_data);
		return -EFAULT;
	}

	/* reset */
	ret = reg_read(dev, INA3221_CONFIG, &reg_data);
	if (ret) {
		return ret;
	}
	reg_data |= INA3221_CONFIG_RST;
	ret = reg_write(dev, INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}

	/* configure */
	reg_data = (cfg->conv_time_shunt << 3) | (cfg->conv_time_bus << 6) | (cfg->avg_mode << 9) |
		   (INA3221_CONFIG_CH1 * cfg->enable_channel[0]) |
		   (INA3221_CONFIG_CH2 * cfg->enable_channel[1]) |
		   (INA3221_CONFIG_CH3 * cfg->enable_channel[2]);

	ret = reg_write(dev, INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int start_measurement(const struct device *dev, bool bus, bool shunt)
{
	int ret;
	uint16_t reg_data;

	ret = reg_read(dev, INA3221_CONFIG, &reg_data);
	if (ret) {
		return ret;
	}

	reg_data &= ~(INA3221_CONFIG_BUS | INA3221_CONFIG_SHUNT);
	reg_data |= (INA3221_CONFIG_BUS * bus) | (INA3221_CONFIG_SHUNT * shunt);

	ret = reg_write(dev, INA3221_CONFIG, reg_data);
	if (ret) {
		return ret;
	}
	return 0;
}

static bool measurement_ready(const struct device *dev)
{
	int ret;
	uint16_t reg_data;

	ret = reg_read(dev, INA3221_MASK_ENABLE, &reg_data);
	if (ret) {
		return false;
	}
	return reg_data & INA3221_MASK_ENABLE_CONVERSION_READY;
}

static int ina3221_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ina3221_data *data = dev->data;
	const struct ina3221_config *cfg = dev->config;
	bool measurement_successful = false;
	k_timeout_t measurement_time = K_NO_WAIT;
	int ret;

	/* Trigger measurement and wait for completion */
	if (chan == SENSOR_CHAN_VOLTAGE) {
		ret = start_measurement(dev, true, false);
		if (ret) {
			return ret;
		}
		measurement_time =
			K_USEC(avg_mode_samples[cfg->avg_mode] *
			       conv_time_us[cfg->conv_time_bus]);
	} else if (chan == SENSOR_CHAN_CURRENT) {
		ret = start_measurement(dev, false, true);
		if (ret) {
			return ret;
		}
		measurement_time =
			K_USEC(avg_mode_samples[cfg->avg_mode] *
			       conv_time_us[cfg->conv_time_shunt]);
	} else if (chan == SENSOR_CHAN_POWER || chan == SENSOR_CHAN_ALL) {
		ret = start_measurement(dev, true, true);
		if (ret) {
			return ret;
		}
		measurement_time =
			K_USEC(avg_mode_samples[cfg->avg_mode] *
			       conv_time_us[MAX(cfg->conv_time_shunt, cfg->conv_time_bus)]);
	} else {
		return -ENOTSUP;
	}

	for (size_t i = 0; i < MAX_RETRIES; ++i) {
		k_sleep(measurement_time);
		if (measurement_ready(dev)) {
			measurement_successful = true;
			break;
		}
	}

	if (!measurement_successful) {
		LOG_ERR("Measurement timed out.");
		return -EFAULT;
	}

	for (size_t i = 0; i < 3; ++i) {
		if (!cfg->enable_channel[i]) {
			continue;
		}
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER ||
		    chan == SENSOR_CHAN_VOLTAGE) {
			ret = reg_read(dev, INA3221_BUS_V1 + 2 * i, &(data->v_bus[i]));
			if (ret) {
				return ret;
			}
			data->v_bus[i] = data->v_bus[i] >> 3;
		}
		if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POWER ||
		    chan == SENSOR_CHAN_CURRENT) {
			ret = reg_read(dev, INA3221_SHUNT_V1 + 2 * i, &(data->v_shunt[i]));
			if (ret) {
				return ret;
			}
			data->v_shunt[i] = data->v_shunt[i] >> 3;
		}
	}

	return ret;
}

static int ina3221_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ina3221_config *cfg = dev->config;
	struct ina3221_data *data = dev->data;
	float result;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		result = data->v_bus[data->selected_channel] * INA3221_BUS_VOLTAGE_LSB;
		break;
	case SENSOR_CHAN_CURRENT:
		result = data->v_shunt[data->selected_channel] * INA3221_SHUNT_VOLTAGE_LSB /
			 (cfg->shunt_r[data->selected_channel] / 1000.0f);
		break;
	case SENSOR_CHAN_POWER:
		result = data->v_bus[data->selected_channel] * INA3221_BUS_VOLTAGE_LSB *
			 data->v_shunt[data->selected_channel] * INA3221_SHUNT_VOLTAGE_LSB /
			 (cfg->shunt_r[data->selected_channel] / 1000.0f);
		break;
	default:
		LOG_DBG("Channel not supported by device!");
		return -ENOTSUP;
	}

	return sensor_value_from_float(val, result);
}

static int ina3221_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	ARG_UNUSED(chan);

	if (attr != SENSOR_ATTR_INA3221_SELECTED_CHANNEL) {
		return -ENOTSUP;
	}

	struct ina3221_data *data = dev->data;

	if (val->val1 < 1 || val->val1 > 3) {
		return -EINVAL;
	}

	data->selected_channel = val->val1 - 1;
	return 0;
}

static const struct sensor_driver_api ina3221_api = {
	.sample_fetch = ina3221_sample_fetch,
	.channel_get = ina3221_channel_get,
	.attr_set = ina3221_attr_set,
};

#define INST_DT_INA3221(index)                                                                     \
	static const struct ina3221_config ina3221_config_##index = {                              \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.avg_mode = DT_INST_PROP(index, avg_mode),                                         \
		.conv_time_bus = DT_INST_PROP(index, conv_time_bus),                               \
		.conv_time_shunt = DT_INST_PROP(index, conv_time_shunt),                           \
		.enable_channel = DT_INST_PROP(index, enable_channel),                             \
		.shunt_r = DT_INST_PROP(index, shunt_resistors),                                   \
	};                                                                                         \
	static struct ina3221_data ina3221_data_##index;                                           \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, ina3221_init, NULL, &ina3221_data_##index,             \
			      &ina3221_config_##index, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
			      &ina3221_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_INA3221);
