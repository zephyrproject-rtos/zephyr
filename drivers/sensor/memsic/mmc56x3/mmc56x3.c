/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include "mmc56x3.h"

LOG_MODULE_REGISTER(MMC56X3, CONFIG_SENSOR_LOG_LEVEL);

K_TIMER_DEFINE(meas_req_timer, NULL, NULL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "MMC56X3 driver enabled without any devices"
#endif

static inline int mmc56x3_bus_check(const struct device *dev)
{
	const struct mmc56x3_dev_config *config = dev->config;

	return config->bus_io->check(&config->bus);
}

static inline int mmc56x3_reg_read(const struct device *dev, uint8_t reg, uint8_t *buf, int size)
{
	const struct mmc56x3_dev_config *config = dev->config;

	return config->bus_io->read(&config->bus, reg, buf, size);
}

static inline int mmc56x3_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mmc56x3_dev_config *config = dev->config;

	return config->bus_io->write(&config->bus, reg, val);
}

static inline int mmc56x3_raw_read(const struct device *dev, uint8_t *buf, int size)
{
	const struct mmc56x3_dev_config *config = dev->config;

	return config->bus_io->raw_read(&config->bus, buf, size);
}

static inline int mmc56x3_raw_write(const struct device *dev, uint8_t *buf, int size)
{
	const struct mmc56x3_dev_config *config = dev->config;

	return config->bus_io->raw_write(&config->bus, buf, size);
}

static int mmc56x3_chip_set_auto_self_reset(const struct device *dev, bool auto_sr)
{
	struct mmc56x3_data *data = dev->data;
	struct mmc56x3_config *config = &data->config;

	if (auto_sr) {
		data->ctrl0_cache |= MMC56X3_CMD_AUTO_SELF_RESET_EN;
	} else {
		data->ctrl0_cache &= ~MMC56X3_CMD_AUTO_SELF_RESET_EN;
	}

	int ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, data->ctrl0_cache);

	if (ret < 0) {
		LOG_DBG("Setting auto_sr_en bit failed: %d", ret);
	} else {
		config->auto_sr = auto_sr;
	}

	return ret;
}

/* Set ODR to 0 to turn off */
static int mmc56x3_chip_set_continuous_mode(const struct device *dev, uint16_t odr)
{
	struct mmc56x3_data *data = dev->data;
	struct mmc56x3_config *config = &data->config;
	int ret;

	if (odr > 255) {
		odr = 1000;
		data->ctrl2_cache |= MMC56X3_CMD_HPOWER;
		ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_ODR, 255);
	} else {
		ret  = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_ODR, (uint8_t) odr);
	}

	if (ret < 0) {
		LOG_DBG("Setting ODR failed: %d", ret);
	} else {
		config->magn_odr = odr;
	}

	if (odr > 0) {
		data->ctrl0_cache |= MMC56X3_CMD_CMM_FREQ_EN;
		data->ctrl2_cache |= MMC56X3_CMD_CMM_EN;
	} else {
		data->ctrl0_cache &= ~MMC56X3_CMD_CMM_FREQ_EN;
		data->ctrl2_cache &= ~MMC56X3_CMD_CMM_EN;
	}

	ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, data->ctrl0_cache);
	if (ret < 0) {
		LOG_DBG("Setting cmm_freq_en bit failed: %d", ret);
	}

	ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_2, data->ctrl2_cache);
	if (ret < 0) {
		LOG_DBG("Setting cmm_en bit failed: %d", ret);
	}

	/* Wait required to get readings normally afterwards */
	k_sleep(K_MSEC(10));

	return ret;
}

static bool mmc56x3_is_continuous_mode(const struct device *dev)
{
	struct mmc56x3_data *data = dev->data;

	return data->ctrl2_cache & MMC56X3_CMD_CMM_EN;
}

int mmc56x3_chip_set_decimation_filter(const struct device *dev, bool bw0, bool bw1)
{
	struct mmc56x3_data *data = dev->data;
	struct mmc56x3_config *config = &data->config;

	data->ctrl1_cache |= (bw0 ? BIT(0) : 0);
	data->ctrl1_cache |= (bw1 ? BIT(1) : 0);

	config->bw0 = bw0;
	config->bw1 = bw1;

	return mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_1, data->ctrl1_cache);
}

static int mmc56x3_chip_init(const struct device *dev)
{
	int ret;
	uint8_t chip_id;

	ret = mmc56x3_bus_check(dev);
	if (ret < 0) {
		LOG_DBG("bus check failed: %d", ret);
		return ret;
	}

	ret = mmc56x3_reg_read(dev, MMC56X3_REG_ID, &chip_id, 1);
	if (ret < 0) {
		LOG_DBG("ID read failed: %d", ret);
		return ret;
	}

	if (chip_id == MMC56X3_CHIP_ID) {
		LOG_DBG("ID OK");
	} else {
		LOG_DBG("bad chip id 0x%x", chip_id);
		return -ENOTSUP;
	}

	ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_1, MMC56X3_CMD_SW_RESET);
	if (ret < 0) {
		LOG_DBG("SW reset failed: %d", ret);
	}

	k_sleep(K_MSEC(20));

	ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, MMC56X3_CMD_SET);
	if (ret < 0) {
		LOG_DBG("Magnetic set failed: %d", ret);
	}

	k_sleep(K_MSEC(1));

	ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, MMC56X3_CMD_RESET);
	if (ret < 0) {
		LOG_DBG("Magnetic reset failed: %d", ret);
	}

	k_sleep(K_MSEC(1));

	struct mmc56x3_data *data = dev->data;
	const struct mmc56x3_config *config = &data->config;

	ret = mmc56x3_chip_set_continuous_mode(dev, config->magn_odr);

	ret = mmc56x3_chip_set_decimation_filter(dev, config->bw0, config->bw1);

	ret = mmc56x3_chip_set_auto_self_reset(dev, config->auto_sr);

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int mmc56x3_wait_until_ready(const struct device *dev)
{
	uint8_t status = 0;
	int ret;

	/* Wait for measurement to be completed */
	do {
		k_sleep(K_MSEC(3));
		ret = mmc56x3_reg_read(dev, MMC56X3_REG_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}
	} while ((status & 0xC0) != (MMC56X3_STATUS_MEAS_M_DONE | MMC56X3_STATUS_MEAS_T_DONE));

	return 0;
}

int mmc56x3_sample_fetch_helper(const struct device *dev, enum sensor_channel chan,
				struct mmc56x3_data *data)
{
	int32_t raw_magn_x, raw_magn_y, raw_magn_z;
	int ret;

	if (!mmc56x3_is_continuous_mode(dev)) {
		/* Temperature cannot be read in continuous mode */
		uint8_t raw_temp;

		ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, MMC56X3_CMD_TAKE_MEAS_T);
		if (ret < 0) {
			return ret;
		}

		k_timer_start(&meas_req_timer, K_MSEC(10), K_NO_WAIT);
		k_timer_status_sync(&meas_req_timer);

		ret = mmc56x3_reg_write(dev, MMC56X3_REG_INTERNAL_CTRL_0, MMC56X3_CMD_TAKE_MEAS_M);
		if (ret < 0) {
			return ret;
		}

		ret = mmc56x3_wait_until_ready(dev);
		if (ret < 0) {
			return ret;
		}

		ret = mmc56x3_reg_read(dev, MMC56X3_REG_TEMP, &raw_temp, 1);
		if (ret < 0) {
			return ret;
		}
		data->temp = raw_temp;
	}

	uint8_t sig = MMC56X3_REG_MAGN_X_OUT_0;

	ret = mmc56x3_raw_write(dev, &sig, 1);
	if (ret < 0) {
		return ret;
	}

	uint8_t buf[9];

	ret = mmc56x3_raw_read(dev, buf, 9);
	if (ret < 0) {
		return ret;
	}

	/* 20-bit precision default */
	raw_magn_x = (uint32_t)buf[0] << 12 | (uint32_t)buf[1] << 4 | (uint32_t)buf[6] >> 4;
	raw_magn_x -= (uint32_t)1 << 19;
	data->magn_x = raw_magn_x;

	raw_magn_y = (uint32_t)buf[2] << 12 | (uint32_t)buf[3] << 4 | (uint32_t)buf[7] >> 4;
	raw_magn_y -= (uint32_t)1 << 19;
	data->magn_y = raw_magn_y;

	raw_magn_z = (uint32_t)buf[4] << 12 | (uint32_t)buf[5] << 4 | (uint32_t)buf[8] >> 4;
	raw_magn_z -= (uint32_t)1 << 19;
	data->magn_z = raw_magn_z;

	return 0;
}

int mmc56x3_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mmc56x3_data *data = dev->data;

	return mmc56x3_sample_fetch_helper(dev, chan, data);
}

static void convert_double_to_sensor_val(double d, struct sensor_value *val)
{
	val->val1 = (int32_t)d;
	int32_t precision = 1000000;

	val->val2 = ((int32_t)(d * precision)) - (val->val1 * precision);
}

static int mmc56x3_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mmc56x3_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		convert_double_to_sensor_val(MMC56X3_TEMP_BASE + (data->temp * MMC56X3_TEMP_RES),
					     val);
		break;
	case SENSOR_CHAN_MAGN_X:
		convert_double_to_sensor_val(data->magn_x * MMC56X3_MAGN_GAUSS_RES, val);
		break;
	case SENSOR_CHAN_MAGN_Y:
		convert_double_to_sensor_val(data->magn_y * MMC56X3_MAGN_GAUSS_RES, val);
		break;
	case SENSOR_CHAN_MAGN_Z:
		convert_double_to_sensor_val(data->magn_z * MMC56X3_MAGN_GAUSS_RES, val);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		convert_double_to_sensor_val(data->magn_x * MMC56X3_MAGN_GAUSS_RES, val);
		convert_double_to_sensor_val(data->magn_y * MMC56X3_MAGN_GAUSS_RES, val + 1);
		convert_double_to_sensor_val(data->magn_z * MMC56X3_MAGN_GAUSS_RES, val + 2);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mmc56x3_chip_configure(const struct device *dev, struct mmc56x3_config *new_config)
{
	struct mmc56x3_data *data = dev->data;
	struct mmc56x3_config *config = &data->config;

	int ret = 0;

	if (new_config->magn_odr != config->magn_odr) {
		ret = mmc56x3_chip_set_continuous_mode(dev, new_config->magn_odr);
	}

	if ((new_config->bw0 != config->bw0) || (new_config->bw1 != config->bw1)) {
		ret = mmc56x3_chip_set_decimation_filter(dev, new_config->bw0, new_config->bw1);
	}

	if (new_config->auto_sr != config->auto_sr) {
		ret = mmc56x3_chip_set_auto_self_reset(dev, config->auto_sr);
	}

	return ret;
}

static int mmc56x3_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct mmc56x3_config new_config = {};
	int ret = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			new_config.magn_odr = val->val1;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_0) {
			new_config.bw0 = val->val1 ? true : false;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_1) {
			new_config.bw1 = val->val1 ? true : false;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_AUTOMATIC_SELF_RESET) {
			new_config.auto_sr = val->val1 ? true : false;
		} else {
			LOG_ERR("Unsupported attribute");
			ret = -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		ret = -EINVAL;
		break;
	}

	if (ret) {
		return ret;
	}

	return mmc56x3_chip_configure(dev, &new_config);
}

static int mmc56x3_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	struct mmc56x3_data *data = dev->data;
	struct mmc56x3_config *config = &data->config;
	int ret = 0;

	__ASSERT_NO_MSG(val != NULL);

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
	case SENSOR_CHAN_MAGN_Z:
	case SENSOR_CHAN_MAGN_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = config->magn_odr;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_0) {
			val->val1 = config->bw0;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_BANDWIDTH_SELECTION_BITS_1) {
			val->val1 = config->bw1;
		} else if ((enum sensor_attribute_mmc56x3)attr ==
			   SENSOR_ATTR_AUTOMATIC_SELF_RESET) {
			val->val1 = config->auto_sr;
		} else {
			LOG_ERR("Unsupported attribute");
			ret = -ENOTSUP;
		}
		break;
	default:
		LOG_ERR("Unsupported channel");
		ret = -EINVAL;
		break;
	}

	return ret;
}

static DEVICE_API(sensor, mmc56x3_api_funcs) = {
	.sample_fetch = mmc56x3_sample_fetch,
	.channel_get = mmc56x3_channel_get,
	.attr_get = mmc56x3_attr_get,
	.attr_set = mmc56x3_attr_set,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = mmc56x3_submit,
	.get_decoder = mmc56x3_get_decoder,
#endif
};

#define MMC56X3_DT_DEV_CONFIG_INIT(inst)                                                           \
	{                                                                                          \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst),                                             \
		.bus_io = &mmc56x3_bus_io_i2c,                                                     \
	}

#define MMC56X3_DT_DATA_CONFIG_INIT(inst)                                                          \
	{                                                                                          \
		.magn_odr = DT_INST_PROP(inst, magn_odr),                                          \
		.bw0 = DT_INST_PROP(inst, bandwidth_selection_bits_0),                             \
		.bw1 = DT_INST_PROP(inst, bandwidth_selection_bits_1),                             \
		.auto_sr = DT_INST_PROP(inst, auto_self_reset),                                    \
	}

#define MMC56X3_DATA_INIT(inst)                                                                    \
	static struct mmc56x3_data mmc56x3_data_##inst = {                                         \
		.config = MMC56X3_DT_DATA_CONFIG_INIT(inst),                                       \
	};

#define MMC56X3_DEFINE(inst)                                                                       \
	MMC56X3_DATA_INIT(inst);                                                                   \
	const static struct mmc56x3_dev_config mmc56x3_dev_config_##inst =  \
		MMC56X3_DT_DEV_CONFIG_INIT(inst);                                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mmc56x3_chip_init, NULL, &mmc56x3_data_##inst,          \
				     &mmc56x3_dev_config_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &mmc56x3_api_funcs);

/* Create the struct device for every status "okay" node in the devicetree. */
DT_INST_FOREACH_STATUS_OKAY(MMC56X3_DEFINE)
