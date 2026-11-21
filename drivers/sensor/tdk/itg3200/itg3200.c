/* Copyright (c) 2026 NBuiLe
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet: PS-ITG-3200A-00-01.4
 *
 */

#define DT_DRV_COMPAT invensense_itg3200

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "itg3200.h"

#define ITG3200_GYRO_SENSITIVITY_NRAD 1214142LL

LOG_MODULE_REGISTER(ITG3200, CONFIG_SENSOR_LOG_LEVEL);

static inline int itg3200_bus_check(const struct device *dev)
{
	const struct itg3200_config *cfg = dev->config;

	return i2c_is_ready_dt(&cfg->i2c) ? 0 : -ENODEV;
}

static inline int itg3200_reg_read(const struct device *dev, uint8_t start, uint8_t *buf, int size)
{
	const struct itg3200_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, start, buf, size);
}

static inline int itg3200_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct itg3200_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

static int itg3200_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct itg3200_data *data = dev->data;
	uint8_t buf[9];
	int ret;

	ret = itg3200_reg_read(dev, ITG3200_INT_STATUS, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to fetch sample: %d", ret);
		return ret;
	}

	if ((buf[0] & 0x01U) == 0U) {
		return -EAGAIN;
	}

	data->sample.raw_temp = sys_get_be16(&buf[1]);
	data->sample.raw_gyro_x = sys_get_be16(&buf[3]);
	data->sample.raw_gyro_y = sys_get_be16(&buf[5]);
	data->sample.raw_gyro_z = sys_get_be16(&buf[7]);

	return 0;
}

static inline void itg3200_convert_gyro(int16_t raw, struct sensor_value *val)
{
	int64_t urad = ((int64_t)raw * ITG3200_GYRO_SENSITIVITY_NRAD) / 1000LL;

	val->val1 = (int32_t)(urad / 1000000LL);
	val->val2 = (int32_t)(urad % 1000000LL);
}

static int itg3200_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct itg3200_data *data = dev->data;
	int64_t tmp;

	switch (chan) {
	case SENSOR_CHAN_GYRO_X:
		itg3200_convert_gyro(data->sample.raw_gyro_x, val);
		break;
	case SENSOR_CHAN_GYRO_Y:
		itg3200_convert_gyro(data->sample.raw_gyro_y, val);
		break;
	case SENSOR_CHAN_GYRO_Z:
		itg3200_convert_gyro(data->sample.raw_gyro_z, val);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		itg3200_convert_gyro(data->sample.raw_gyro_x, &val[0]);
		itg3200_convert_gyro(data->sample.raw_gyro_y, &val[1]);
		itg3200_convert_gyro(data->sample.raw_gyro_z, &val[2]);
		break;
	case SENSOR_CHAN_DIE_TEMP:
		/* Temp(C) = 35 + (raw_temp + 13200) / 280 */
		tmp = ((int64_t)(data->sample.raw_temp + 13200) * 1000000LL) / 280LL + 35000000LL;
		val->val1 = (int32_t)(tmp / 1000000LL);
		val->val2 = (int32_t)(tmp % 1000000LL);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int itg3200_pm_action(const struct device *dev, enum pm_device_action action)
{
	/* TBI */
	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, itg3200_api) = {
	.sample_fetch = itg3200_sample_fetch,
	.channel_get = itg3200_channel_get,
};

static int itg3200_init(const struct device *dev)
{
	uint8_t val = 0U;
	int ret;
	struct itg3200_data *itg3200 = dev->data;

	if (itg3200_bus_check(dev) < 0) {
		LOG_DBG("bus check failed");
		return -ENODEV;
	}

	/* 1. Wake up + select PLL */
	val = itg3200->pwr_mgm;
	ret = itg3200_reg_write(dev, ITG3200_PWR_MGM, val);
	if (ret < 0) {
		LOG_ERR("Cannot wakeup chip: %d", ret);
		return ret;
	}

	/* 2. Full-scale ±2000 dps + DLPF */
	val = ((itg3200->fs_sel & 0x03U) << 3) | (itg3200->dlpf_cfg & 0x07U);
	ret = itg3200_reg_write(dev, ITG3200_DLPF_FS, val);
	if (ret < 0) {
		LOG_ERR("Cannot setting filter: %d", ret);
		return ret;
	}

	/* 3. Sample rate */
	val = itg3200->smplrt_div;
	ret = itg3200_reg_write(dev, ITG3200_SMPLRT_DIV, val);
	if (ret < 0) {
		LOG_ERR("Cannot setting Sample rate: %d", ret);
		return ret;
	}

	/* 4. Interrupt configuration */
	val = itg3200->int_cfg;
	ret = itg3200_reg_write(dev, ITG3200_INT_CFG, val);
	if (ret < 0) {
		LOG_ERR("Cannot setting interrupt config: %d", ret);
		return ret;
	}

	return ret;
}

#define ITG3200_INST(inst)                                                                         \
	static struct itg3200_data itg3200_data_##inst = {                                         \
		.smplrt_div = DT_INST_PROP(inst, smplrt_div),                                      \
		.fs_sel = DT_INST_PROP(inst, fs_sel),                                              \
		.dlpf_cfg = DT_INST_PROP(inst, dlpf_cfg),                                          \
		.int_cfg = DT_INST_PROP(inst, int_cfg),                                            \
		.pwr_mgm = DT_INST_PROP(inst, pwr_mgm),                                            \
	};                                                                                         \
	static const struct itg3200_config itg3200_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, itg3200_pm_action);                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, itg3200_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &itg3200_data_##inst, &itg3200_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &itg3200_api);

DT_INST_FOREACH_STATUS_OKAY(ITG3200_INST)
