/*
 * Copyright (c) 2024 Gustavo Silva
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sciosense_ens160

#include <zephyr/drivers/sensor/ens160.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include "ens160.h"

LOG_MODULE_REGISTER(ENS160, CONFIG_SENSOR_LOG_LEVEL);

static int ens160_set_temperature(const struct device *dev, const struct sensor_value *val)
{
	struct ens160_data *data = dev->data;
	uint8_t buf[2];
	int64_t temp;
	int ret;

	/* Recommended operation: -5 to 60 degrees Celsius */
	if (!IN_RANGE(val->val1, -5, 60)) {
		LOG_ERR("Invalid temperature value");
		return -EINVAL;
	}

	/* Convert temperature from Celsius to Kelvin */
	temp = sensor_value_to_micro(val) + 273150000U;
	/* Temperature is stored in 64 * Kelvin */
	temp *= 64;
	sys_put_le16(DIV_ROUND_CLOSEST(temp, 1000000U), buf);

	ret = data->tf->write_data(dev, ENS160_REG_TEMP_IN, buf, 2U);
	if (ret < 0) {
		LOG_ERR("Failed to write temperature");
		return ret;
	}

	return 0;
}

static int ens160_set_humidity(const struct device *dev, const struct sensor_value *val)
{
	struct ens160_data *data = dev->data;
	uint8_t buf[2];
	uint64_t rh;
	int ret;

	/* Recommended operation: 20 to 80% RH */
	if (!IN_RANGE(val->val1, 20, 80)) {
		LOG_ERR("Invalid RH value");
		return -EINVAL;
	}

	rh = sensor_value_to_micro(val);
	/* RH value is stored in 512 * %RH */
	rh *= 512;
	sys_put_le16(DIV_ROUND_CLOSEST(rh, 1000000U), buf);

	ret = data->tf->write_data(dev, ENS160_REG_RH_IN, buf, 2U);
	if (ret < 0) {
		LOG_ERR("Failed to write RH");
		return ret;
	}

	return 0;
}

static bool ens160_new_data(const struct device *dev)
{
	struct ens160_data *data = dev->data;
	uint8_t status;
	int ret;

	ret = data->tf->read_reg(dev, ENS160_REG_DEVICE_STATUS, &status);
	if (ret < 0) {
		return ret;
	}

	return FIELD_GET(ENS160_STATUS_NEWDAT, status) != 0;
}

static int ens160_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ens160_data *data = dev->data;
	uint16_t le16_buffer;
	uint8_t buffer;
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_CO2 ||
			chan == SENSOR_CHAN_VOC ||
			chan == (enum sensor_channel)SENSOR_CHAN_ENS160_AQI);

	if (!IS_ENABLED(CONFIG_ENS160_TRIGGER)) {
		WAIT_FOR(ens160_new_data(dev), ENS160_TIMEOUT_US, k_msleep(10));
	}

	ret = data->tf->read_data(dev, ENS160_REG_DATA_ECO2, (uint8_t *)&le16_buffer,
				  sizeof(le16_buffer));
	if (ret < 0) {
		LOG_ERR("Failed to fetch CO2");
		return ret;
	}

	data->eco2 = sys_le16_to_cpu(le16_buffer);

	ret = data->tf->read_data(dev, ENS160_REG_DATA_TVOC, (uint8_t *)&le16_buffer,
				  sizeof(le16_buffer));
	if (ret < 0) {
		LOG_ERR("Failed to fetch VOC");
		return ret;
	}

	data->tvoc = sys_le16_to_cpu(le16_buffer);

	ret = data->tf->read_reg(dev, ENS160_REG_DATA_AQI, &buffer);
	if (ret < 0) {
		LOG_ERR("Failed to fetch AQI");
		return ret;
	}

	data->aqi = FIELD_GET(ENS160_DATA_AQI_UBA, buffer);

	return 0;
}

static int ens160_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ens160_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CO2:
		val->val1 = data->eco2;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_VOC:
		val->val1 = data->tvoc;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_ENS160_AQI:
		val->val1 = data->aqi;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ens160_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret = 0;

	switch ((uint32_t)attr) {
	case SENSOR_ATTR_ENS160_TEMP:
		ret = ens160_set_temperature(dev, val);
		break;
	case SENSOR_ATTR_ENS160_RH:
		ret = ens160_set_humidity(dev, val);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static DEVICE_API(sensor, ens160_driver_api) = {
	.sample_fetch = ens160_sample_fetch,
	.channel_get = ens160_channel_get,
	.attr_set = ens160_attr_set,
#ifdef CONFIG_ENS160_TRIGGER
	.trigger_set = ens160_trigger_set,
#endif
};

static int ens160_init(const struct device *dev)
{
	const struct ens160_config *config = dev->config;
	struct ens160_data *data = dev->data;
	uint8_t fw_version[3];
	uint16_t part_id;
	uint8_t status;
	int ret;

	ret = config->bus_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device");
		return ret;
	}

	k_msleep(ENS160_BOOTING_TIME_MS);

	ret = data->tf->read_data(dev, ENS160_REG_PART_ID, (uint8_t *)&part_id, sizeof(part_id));
	if (ret < 0) {
		LOG_ERR("Failed to read Part ID");
		return -EIO;
	}

	if (sys_le16_to_cpu(part_id) != ENS160_PART_ID) {
		LOG_ERR("Part ID is invalid. Expected: 0x%x; read: 0x%x", ENS160_PART_ID, part_id);
		return -EIO;
	}

	ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_IDLE);
	if (ret < 0) {
		LOG_ERR("Failed to set operation mode");
		return ret;
	}

	k_msleep(ENS160_BOOTING_TIME_MS);

	ret = data->tf->write_reg(dev, ENS160_REG_COMMAND, ENS160_COMMAND_CLRGPR);
	if (ret < 0) {
		LOG_ERR("Failed to clear GPR registers");
		return ret;
	}

	ret = data->tf->write_reg(dev, ENS160_REG_COMMAND, ENS160_COMMAND_GET_APPVER);
	if (ret < 0) {
		LOG_ERR("Failed to write GET_APPVER command");
		return ret;
	}

	k_msleep(ENS160_BOOTING_TIME_MS);

	ret = data->tf->read_data(dev, ENS160_REG_GPR_READ4, fw_version, sizeof(fw_version));
	if (ret < 0) {
		LOG_ERR("Failed to read firmware version");
		return ret;
	}
	LOG_INF("Firmware version: %u.%u.%u", fw_version[2], fw_version[1], fw_version[0]);

#ifdef CONFIG_ENS160_TRIGGER
	ret = ens160_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt");
		return ret;
	}
#endif /* CONFIG_ENS160_TRIGGER */

	ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_STANDARD);
	if (ret < 0) {
		LOG_ERR("Failed to set operation mode");
		return ret;
	}

	k_msleep(ENS160_BOOTING_TIME_MS);

	ret = data->tf->read_reg(dev, ENS160_REG_DEVICE_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to read device status");
		return ret;
	}

	if (FIELD_GET(ENS160_STATUS_VALIDITY_FLAG, status) != ENS160_STATUS_NORMAL) {
		LOG_ERR("Status 0x%02x is invalid", status);
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int ens160_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct ens160_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_IDLE);
		k_msleep(ENS160_BOOTING_TIME_MS);
		ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_STANDARD);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = data->tf->write_reg(dev, ENS160_REG_OPMODE, ENS160_OPMODE_DEEP_SLEEP);
		break;
	default:
		return -ENOTSUP;
	}

	k_msleep(ENS160_BOOTING_TIME_MS);

	return ret;
}
#endif

#define ENS160_SPI_OPERATION                                                                       \
	(SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_TRANSFER_MSB)

#define ENS160_CONFIG_SPI(inst)                                                                    \
	.bus_init = &ens160_spi_init,                                                              \
	.spi = SPI_DT_SPEC_INST_GET(inst, ENS160_SPI_OPERATION, 0),

#define ENS160_CONFIG_I2C(inst)                                                                    \
	.bus_init = &ens160_i2c_init,                                                              \
	.i2c = I2C_DT_SPEC_INST_GET(inst),

#define ENS160_DEFINE(inst)                                                                        \
	static struct ens160_data ens160_data_##inst;                                              \
	static const struct ens160_config ens160_config_##inst = {                                 \
		IF_ENABLED(CONFIG_ENS160_TRIGGER,                                                  \
			  (.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),))                   \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),                                             \
			   (ENS160_CONFIG_SPI(inst)),                                              \
			   (ENS160_CONFIG_I2C(inst)))                                              \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, ens160_pm_action);                                          \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ens160_init, PM_DEVICE_DT_INST_GET(inst),               \
				     &ens160_data_##inst, &ens160_config_##inst, POST_KERNEL,      \
				     CONFIG_SENSOR_INIT_PRIORITY, &ens160_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ENS160_DEFINE)
