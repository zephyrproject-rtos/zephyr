/*
 * Copyright (c) 2023 Kurtis Dinelle
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tsl2591

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include "tsl2591.h"

LOG_MODULE_REGISTER(TSL2591, CONFIG_SENSOR_LOG_LEVEL);

static int tsl2591_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint8_t size)
{
	const struct tsl2591_config *config = dev->config;
	uint8_t cmd = TSL2591_NORMAL_CMD | reg;

	return i2c_write_read_dt(&config->i2c, &cmd, 1U, buf, size);
}

static int tsl2591_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct tsl2591_config *config = dev->config;
	uint8_t cmd[2] = {TSL2591_NORMAL_CMD | reg, val};

	return i2c_write_dt(&config->i2c, cmd, 2U);
}

int tsl2591_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	uint8_t old_value, new_value;
	int ret;

	ret = tsl2591_reg_read(dev, reg, &old_value, 1U);
	if (ret < 0) {
		return ret;
	}

	new_value = (old_value & ~mask) | (val & mask);
	if (new_value == old_value) {
		return 0;
	}

	return tsl2591_reg_write(dev, reg, new_value);
}

static int tsl2591_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct tsl2591_data *data = dev->data;
	uint8_t als_data[4];
	int ret;

#ifdef CONFIG_TSL2591_FETCH_WAIT
	uint8_t status;

	ret = tsl2591_reg_read(dev, TSL2591_REG_STATUS, &status, 1U);
	if (ret < 0) {
		LOG_ERR("Failed to read status register");
		return ret;
	}

	/* Check if ALS has completed an integration cycle since AEN asserted.
	 * If not, sleep for the duration of an integration cycle to ensure valid reading.
	 */
	if (!(status & TSL2591_AVALID_MASK)) {
		k_msleep((data->atime / 100) * TSL2591_MAX_TIME_STEP);
	}

	/* Reassert AEN to determine if next reading is valid */
	ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_AEN_MASK, TSL2591_AEN_OFF);
	if (ret < 0) {
		LOG_ERR("Failed to disable ALS");
		return ret;
	}

	ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_AEN_MASK, TSL2591_AEN_ON);
	if (ret < 0) {
		LOG_ERR("Failed to re-enable ALS");
		return ret;
	}
#endif

	switch (chan) {
	case SENSOR_CHAN_ALL:
		ret = tsl2591_reg_read(dev, TSL2591_REG_C0DATAL, als_data, 4U);
		if (ret < 0) {
			LOG_ERR("Failed to read ALS data");
			return ret;
		}

		data->vis_count = sys_get_le16(als_data);
		data->ir_count = sys_get_le16(als_data + 2);
		break;
	case SENSOR_CHAN_LIGHT:
		ret = tsl2591_reg_read(dev, TSL2591_REG_C0DATAL, als_data, 2U);
		if (ret < 0) {
			LOG_ERR("Failed to read ALS visible light data");
			return ret;
		}

		data->vis_count = sys_get_le16(als_data);
		break;
	case SENSOR_CHAN_IR:
		ret = tsl2591_reg_read(dev, TSL2591_REG_C1DATAL, als_data, 2U);
		if (ret < 0) {
			LOG_ERR("Failed to read ALS infrared data");
			return ret;
		}

		data->ir_count = sys_get_le16(als_data);
		break;
	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

#ifdef CONFIG_TSL2591_WARN_SATURATED
	uint16_t max_count = data->atime == 100 ? TSL2591_MAX_ADC_100 : TSL2591_MAX_ADC;
	bool vis_saturated = (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) &&
			     (data->vis_count >= max_count);
	bool ir_saturated = (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_IR) &&
			    (data->ir_count >= max_count);
	if (vis_saturated || ir_saturated) {
		LOG_WRN("Sensor ADC potentially saturated, reading may be invalid");
		return -EOVERFLOW;
	}
#endif

	return 0;
}

static int tsl2591_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct tsl2591_data *data = dev->data;
	int64_t cpl = (int64_t)data->atime * (int64_t)data->again;
	int64_t strength;

	/* Unfortunately, datasheet does not provide a lux conversion formula for this particular
	 * device. There is still ongoing discussion about the proper formula, though this
	 * implementation uses a slightly modified version of the Adafruit library formula:
	 * https://github.com/adafruit/Adafruit_TSL2591_Library/
	 *
	 * Since the device relies on both visible and IR readings to calculate lux,
	 * read SENSOR_CHAN_ALL to get a closer approximation of lux. Reading SENSOR_CHAN_LIGHT or
	 * SENSOR_CHAN_IR individually can be more closely thought of as relative strength
	 * as opposed to true lux.
	 */
	switch (chan) {
	case SENSOR_CHAN_ALL:
		if (data->vis_count > 0) {
			cpl *= 1000000;
			strength =
				(data->vis_count - data->ir_count) *
				(1000000 - (((int64_t)data->ir_count * 1000000) / data->vis_count));
		} else {
			strength = 0;
		}
		break;
	case SENSOR_CHAN_LIGHT:
		strength = data->vis_count;
		break;
	case SENSOR_CHAN_IR:
		strength = data->ir_count;
		break;
	default:
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	strength *= TSL2591_LUX_DF;
	val->val1 = strength / cpl;
	val->val2 = ((strength % cpl) * 1000000) / cpl;

	return 0;
}

#ifdef CONFIG_TSL2591_TRIGGER
static int tsl2591_set_threshold(const struct device *dev, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	const struct tsl2591_data *data = dev->data;
	const struct tsl2591_config *config = dev->config;
	uint64_t cpl;
	uint32_t raw;
	uint16_t thld;
	uint8_t thld_reg;
	uint8_t cmd[3];
	int ret;

	/* Convert from relative strength of visible light to raw value */
	cpl = data->atime * data->again;
	raw = ((val->val1 * cpl) / TSL2591_LUX_DF) +
	      ((val->val2 * cpl) / (1000000U * TSL2591_LUX_DF));

	if (raw > TSL2591_MAX_ADC) {
		LOG_ERR("Given value would overflow threshold register");
		return -EOVERFLOW;
	}

	thld = sys_cpu_to_le16(raw);
	thld_reg = attr == SENSOR_ATTR_LOWER_THRESH ? TSL2591_REG_AILTL : TSL2591_REG_AIHTL;

	cmd[0] = TSL2591_NORMAL_CMD | thld_reg;
	bytecpy(cmd + 1, &thld, 2U);

	ret = i2c_write_dt(&config->i2c, cmd, 3U);
	if (ret < 0) {
		LOG_ERR("Failed to set interrupt threshold");
	}

	return ret;
}

static int tsl2591_set_persist(const struct device *dev, int32_t persist_filter)
{
	uint8_t persist_mode;
	int ret;

	switch (persist_filter) {
	case 0:
		persist_mode = TSL2591_PERSIST_EVERY;
		break;
	case 1:
		persist_mode = TSL2591_PERSIST_1;
		break;
	case 2:
		persist_mode = TSL2591_PERSIST_2;
		break;
	case 3:
		persist_mode = TSL2591_PERSIST_3;
		break;
	case 5:
		persist_mode = TSL2591_PERSIST_5;
		break;
	case 10:
		persist_mode = TSL2591_PERSIST_10;
		break;
	case 15:
		persist_mode = TSL2591_PERSIST_15;
		break;
	case 20:
		persist_mode = TSL2591_PERSIST_20;
		break;
	case 25:
		persist_mode = TSL2591_PERSIST_25;
		break;
	case 30:
		persist_mode = TSL2591_PERSIST_30;
		break;
	case 35:
		persist_mode = TSL2591_PERSIST_35;
		break;
	case 40:
		persist_mode = TSL2591_PERSIST_40;
		break;
	case 45:
		persist_mode = TSL2591_PERSIST_45;
		break;
	case 50:
		persist_mode = TSL2591_PERSIST_50;
		break;
	case 55:
		persist_mode = TSL2591_PERSIST_55;
		break;
	case 60:
		persist_mode = TSL2591_PERSIST_60;
		break;
	default:
		LOG_ERR("Invalid persist filter");
		return -EINVAL;
	}

	ret = tsl2591_reg_write(dev, TSL2591_REG_PERSIST, persist_mode);
	if (ret < 0) {
		LOG_ERR("Failed to set persist filter");
	}

	return ret;
}
#endif

static int tsl2591_set_gain(const struct device *dev, enum sensor_gain_tsl2591 gain)
{
	struct tsl2591_data *data = dev->data;
	uint8_t gain_mode;
	int ret;

	switch (gain) {
	case TSL2591_SENSOR_GAIN_LOW:
		data->again = TSL2591_GAIN_SCALE_LOW;
		gain_mode = TSL2591_GAIN_MODE_LOW;
		break;
	case TSL2591_SENSOR_GAIN_MED:
		data->again = TSL2591_GAIN_SCALE_MED;
		gain_mode = TSL2591_GAIN_MODE_MED;
		break;
	case TSL2591_SENSOR_GAIN_HIGH:
		data->again = TSL2591_GAIN_SCALE_HIGH;
		gain_mode = TSL2591_GAIN_MODE_HIGH;
		break;
	case TSL2591_SENSOR_GAIN_MAX:
		data->again = TSL2591_GAIN_SCALE_MAX;
		gain_mode = TSL2591_GAIN_MODE_MAX;
		break;
	default:
		LOG_ERR("Invalid gain mode");
		return -EINVAL;
	}

	ret = tsl2591_reg_update(dev, TSL2591_REG_CONFIG, TSL2591_AGAIN_MASK, gain_mode);
	if (ret < 0) {
		LOG_ERR("Failed to set gain mode");
	}

	return ret;
}

static int tsl2591_set_integration(const struct device *dev, int32_t integration_time)
{
	struct tsl2591_data *data = dev->data;
	uint8_t atime_mode;
	int ret;

	switch (integration_time) {
	case 100:
		atime_mode = TSL2591_INTEGRATION_100MS;
		break;
	case 200:
		atime_mode = TSL2591_INTEGRATION_200MS;
		break;
	case 300:
		atime_mode = TSL2591_INTEGRATION_300MS;
		break;
	case 400:
		atime_mode = TSL2591_INTEGRATION_400MS;
		break;
	case 500:
		atime_mode = TSL2591_INTEGRATION_500MS;
		break;
	case 600:
		atime_mode = TSL2591_INTEGRATION_600MS;
		break;
	default:
		LOG_ERR("Invalid integration time");
		return -EINVAL;
	}

	ret = tsl2591_reg_update(dev, TSL2591_REG_CONFIG, TSL2591_ATIME_MASK, atime_mode);
	if (ret < 0) {
		LOG_ERR("Failed to set integration time");
		return ret;
	}

	data->atime = integration_time;

	return 0;
}

static int tsl2591_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct tsl2591_data *data = dev->data;
	int ret;

	ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_POWER_MASK, TSL2591_POWER_OFF);
	if (ret < 0) {
		LOG_ERR("Unable to power down device");
		return ret;
	}

#ifdef CONFIG_TSL2591_TRIGGER
	if (attr == SENSOR_ATTR_UPPER_THRESH || attr == SENSOR_ATTR_LOWER_THRESH) {
		if (chan == SENSOR_CHAN_LIGHT) {
			ret = tsl2591_set_threshold(dev, attr, val);
		} else {
			LOG_ERR("Attribute not supported for channel");
			ret = -ENOTSUP;
		}
		goto exit;
	}
#endif

	switch ((enum sensor_attribute_tsl2591)attr) {
	case SENSOR_ATTR_GAIN_MODE:
		ret = tsl2591_set_gain(dev, (enum sensor_gain_tsl2591)val->val1);
		break;
	case SENSOR_ATTR_INTEGRATION_TIME:
		ret = tsl2591_set_integration(dev, val->val1);
		break;

#ifdef CONFIG_TSL2591_TRIGGER
	case SENSOR_ATTR_INT_PERSIST:
		ret = tsl2591_set_persist(dev, val->val1);
		break;
#endif
	default:
		LOG_ERR("Invalid sensor attribute");
		ret = -EINVAL;
		goto exit; /* So the compiler doesn't warn if triggers not enabled */
	}

exit:
	if (data->powered_on) {
		ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_POWER_MASK,
					 TSL2591_POWER_ON);
	}

	return ret;
}

static int tsl2591_setup(const struct device *dev)
{
	struct tsl2591_data *data = dev->data;
	uint8_t device_id;
	int ret;

	ret = tsl2591_reg_write(dev, TSL2591_REG_CONFIG, TSL2591_SRESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset device");
		return ret;
	}

	ret = tsl2591_reg_read(dev, TSL2591_REG_ID, &device_id, 1U);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID");
		return ret;
	}

	if (device_id != TSL2591_DEV_ID) {
		LOG_ERR("Device with ID 0x%02x is not supported", device_id);
		return -ENOTSUP;
	}

	/* Set initial values to match sensor values on reset */
	data->again = TSL2591_GAIN_SCALE_LOW;
	data->atime = 100U;

	ret = tsl2591_reg_write(dev, TSL2591_REG_ENABLE, TSL2591_POWER_ON);
	if (ret < 0) {
		LOG_ERR("Failed to perform initial power up of device");
		return ret;
	}

	data->powered_on = true;

	return 0;
}

static int tsl2591_init(const struct device *dev)
{
	const struct tsl2591_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C dev %s not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	ret = tsl2591_setup(dev);
	if (ret < 0) {
		LOG_ERR("Failed to setup device");
		return ret;
	}

#ifdef CONFIG_TSL2591_TRIGGER
	ret = tsl2591_initialize_int(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, tsl2591_driver_api) = {
#ifdef CONFIG_TSL2591_TRIGGER
	.trigger_set = tsl2591_trigger_set,
#endif
	.attr_set = tsl2591_attr_set,
	.sample_fetch = tsl2591_sample_fetch,
	.channel_get = tsl2591_channel_get};

#ifdef CONFIG_PM_DEVICE
static int tsl2591_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct tsl2591_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_POWER_MASK,
					 TSL2591_POWER_ON);
		if (ret < 0) {
			LOG_ERR("Failed to power on device");
			return ret;
		}

		data->powered_on = true;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = tsl2591_reg_update(dev, TSL2591_REG_ENABLE, TSL2591_POWER_MASK,
					 TSL2591_POWER_OFF);
		if (ret < 0) {
			LOG_ERR("Failed to power off device");
			return ret;
		}

		data->powered_on = false;
		break;
	default:
		LOG_ERR("Unsupported PM action");
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define TSL2591_INIT_INST(n)                                                                       \
	static struct tsl2591_data tsl2591_data_##n;                                               \
	static const struct tsl2591_config tsl2591_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		IF_ENABLED(CONFIG_TSL2591_TRIGGER,                                                 \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(n, int_gpios, {0}),))};           \
	PM_DEVICE_DT_INST_DEFINE(n, tsl2591_pm_action);                                            \
	SENSOR_DEVICE_DT_INST_DEFINE(n, tsl2591_init, PM_DEVICE_DT_INST_GET(n), &tsl2591_data_##n, \
				     &tsl2591_config_##n, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &tsl2591_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSL2591_INIT_INST)
