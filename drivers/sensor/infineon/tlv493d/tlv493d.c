/*
 * Copyright (c) 2026 Junseo Pyun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tlv493d

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
#include "tlv493d.h"

static const uint8_t tlv493d_mode_map[] = {
	[TLV493D_OP_MODE_POWERDOWN] = 0b00,        [TLV493D_OP_MODE_FAST] = 0b10,
	[TLV493D_OP_MODE_LOWPOWER] = 0b01,         [TLV493D_OP_MODE_ULTRA_LOWPOWER] = 0b01,
	[TLV493D_OP_MODE_MASTERCONTROLLED] = 0b11,
};

static const uint8_t tlv493d_lp_period_map[] = {
	[TLV493D_OP_MODE_POWERDOWN] = 0,        [TLV493D_OP_MODE_FAST] = 0,
	[TLV493D_OP_MODE_LOWPOWER] = 1,         [TLV493D_OP_MODE_ULTRA_LOWPOWER] = 0,
	[TLV493D_OP_MODE_MASTERCONTROLLED] = 0,
};

LOG_MODULE_REGISTER(TLV493D, CONFIG_SENSOR_LOG_LEVEL);

static int tlv493d_read_regs(const struct device *dev)
{
	const struct tlv493d_config *cfg = dev->config;
	struct tlv493d_data *data = dev->data;

	int ret = i2c_read_dt(&cfg->i2c, data->rd_regs, sizeof(data->rd_regs));

	if (ret < 0) {
		LOG_ERR("Failed to read sensor registers: %d", ret);
		return ret;
	}

	return 0;
}

static int tlv493d_write_config(const struct device *dev)
{
	const struct tlv493d_config *cfg = dev->config;
	struct tlv493d_data *data = dev->data;

	int ret = i2c_write_dt(&cfg->i2c, data->wr_regs, sizeof(data->wr_regs));

	if (ret < 0) {
		LOG_ERR("Failed to write sensor configuration: %d", ret);
		return ret;
	}

	return 0;
}

static int tlv493d_fetch(const struct device *dev)
{
	struct tlv493d_data *data = dev->data;
	const struct tlv493d_config *cfg = dev->config;

	if (cfg->mode == TLV493D_OP_MODE_POWERDOWN) {
		LOG_ERR("Cannot fetch sample while sensor is in powerdown mode");
		return -ENODATA;
	}

	int ret = tlv493d_read_regs(dev);

	if (ret < 0) {
		return ret;
	}

	uint8_t *regs = data->rd_regs;

	uint16_t x = FIELD_GET(TLV493D_X_MAG_X_AXIS_MSB, regs[TLV493D_RD_REG_X]) << 4 |
		     FIELD_GET(TLV493D_X2_MAG_X_AXIS_LSB, regs[TLV493D_RD_REG_X2]);
	uint16_t y = FIELD_GET(TLV493D_Y_MAG_Y_AXIS_MSB, regs[TLV493D_RD_REG_Y]) << 4 |
		     FIELD_GET(TLV493D_X2_MAG_Y_AXIS_LSB, regs[TLV493D_RD_REG_X2]);
	uint16_t z = FIELD_GET(TLV493D_Z_MAG_Z_AXIS_MSB, regs[TLV493D_RD_REG_Z]) << 4 |
		     FIELD_GET(TLV493D_Z2_MAG_Z_AXIS_LSB, regs[TLV493D_RD_REG_Z2]);
	uint16_t temp = (FIELD_GET(TLV493D_TEMP_TEMP_MSB, regs[TLV493D_RD_REG_TEMP]) << 8) |
			FIELD_GET(TLV493D_TEMP2_TEMP_LSB, regs[TLV493D_RD_REG_TEMP2]);
	data->x = sign_extend(x, 11);
	data->y = sign_extend(y, 11);
	data->z = sign_extend(z, 11);
	data->t = sign_extend(temp, 11);

	LOG_DBG("x: %d, y: %d, z:%d, t:%d\n", data->x, data->y, data->z, data->t);

	return 0;
}

static void tlv493d_convert_to_sensor_value(int16_t raw, struct sensor_value *val)
{
	/* Convert raw magnetic field value (0.098 mT/LSB) to Gauss. */
	int64_t value = (int64_t)raw * TLV493D_MAG_SCALE_NUM * TLV493D_CONVERSION_FACTOR /
			TLV493D_MAG_SCALE_DEN;

	val->val1 = value / TLV493D_CONVERSION_FACTOR;
	val->val2 = value % TLV493D_CONVERSION_FACTOR;
}

static void tlv493d_temperature_to_sensor_value(int16_t temperature, struct sensor_value *val)
{
	/*
	 * Convert raw die temperature value to degrees Celsius.
	 * Temp = (raw - offset) * 1.1 + 25°C
	 */
	int64_t temp = ((int64_t)(temperature - TLV493D_TEMP_OFFSET) * TLV493D_TEMP_SCALE_NUM *
			TLV493D_CONVERSION_FACTOR) /
		       TLV493D_TEMP_SCALE_DEN;
	temp += (int64_t)TLV493D_TEMP_REF * TLV493D_CONVERSION_FACTOR;
	val->val1 = temp / TLV493D_CONVERSION_FACTOR;
	val->val2 = temp % TLV493D_CONVERSION_FACTOR;
}

static int tlv493d_set_operating_mode(uint8_t *wr_regs, enum tlv493d_op_mode mode)
{
	if (mode < TLV493D_OP_MODE_POWERDOWN || mode > TLV493D_OP_MODE_MASTERCONTROLLED) {
		LOG_ERR("Unsupported operating mode: %d", mode);
		return -EINVAL;
	}

	uint8_t *mode1_cfg = &wr_regs[TLV493D_WR_REG_MODE1];
	uint8_t *mode2_cfg = &wr_regs[TLV493D_WR_REG_MODE2];
	uint8_t mode_map = tlv493d_mode_map[mode];
	uint8_t lp_map = tlv493d_lp_period_map[mode];

	WRITE_BIT(*mode1_cfg, 0, !!(mode_map & BIT(0)));
	WRITE_BIT(*mode1_cfg, 1, !!(mode_map & BIT(1)));
	WRITE_BIT(*mode2_cfg, 6, !!(lp_map & BIT(0)));

	return 0;
}

int tlv493d_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	return tlv493d_fetch(dev);
}

int tlv493d_channel_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val)
{
	const struct tlv493d_config *cfg = dev->config;
	struct tlv493d_data *data = dev->data;
	int32_t raw_value;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		raw_value = data->x;
		break;
	case SENSOR_CHAN_MAGN_Y:
		raw_value = data->y;
		break;
	case SENSOR_CHAN_MAGN_Z:
		raw_value = data->z;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		if (cfg->temp_disable) {
			LOG_ERR("Cannot fetch sample while temperature measurement is disabled");
			return -ENODATA;
		}

		tlv493d_temperature_to_sensor_value(data->t, val);
		return 0;
	default:
		return -ENOTSUP;
	}

	tlv493d_convert_to_sensor_value(raw_value, val);
	return 0;
}

int tlv493d_init(const struct device *dev)
{
	const struct tlv493d_config *cfg = dev->config;
	struct tlv493d_data *data = dev->data;

	int ret = 0;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = tlv493d_read_regs(dev);
	if (ret < 0) {
		return ret;
	}

	data->mode = cfg->mode;
	data->wr_regs[0] = 0;
	data->wr_regs[1] = data->rd_regs[7] & TLV493D_RD_REG_RES1_WR_MASK;
	data->wr_regs[2] = data->rd_regs[8] & TLV493D_RD_REG_RES2_WR_MASK;
	data->wr_regs[3] = data->rd_regs[9] & TLV493D_RD_REG_RES3_WR_MASK;
	WRITE_BIT(data->wr_regs[TLV493D_WR_REG_MODE2], 7, cfg->temp_disable);

	ret = tlv493d_set_operating_mode(data->wr_regs, data->mode);
	if (ret < 0) {
		return ret;
	}

	ret = tlv493d_write_config(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);

	if (ret) {
		LOG_ERR("Failed to enable runtime PM");
		return ret;
	}
#endif

	return ret;
}

static int tlv493d_mode_to_sampling_frequency(enum tlv493d_op_mode mode, struct sensor_value *val)
{
	switch (mode) {
	case TLV493D_OP_MODE_POWERDOWN:
		val->val1 = 0;
		val->val2 = 0;
		break;

	case TLV493D_OP_MODE_FAST:
		val->val1 = 3300;
		val->val2 = 0;
		break;

	case TLV493D_OP_MODE_LOWPOWER:
		val->val1 = 100;
		val->val2 = 0;
		break;

	case TLV493D_OP_MODE_ULTRA_LOWPOWER:
		val->val1 = 10;
		val->val2 = 0;
		break;

	case TLV493D_OP_MODE_MASTERCONTROLLED:
		val->val1 = 0;
		val->val2 = 0;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int tlv493d_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	ARG_UNUSED(chan);
	struct tlv493d_data *data = dev->data;
	int ret = tlv493d_read_regs(dev);

	if (ret < 0) {
		return ret;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return tlv493d_mode_to_sampling_frequency(data->mode, val);
	default:
		return -ENOTSUP;
	}
}

static int tlv493d_get_mode_from_odr(const struct sensor_value *freq, enum tlv493d_op_mode *mode)
{
	switch (freq->val1) {
	case 0:
		*mode = TLV493D_OP_MODE_MASTERCONTROLLED;
		return 0;

	case 10:
		*mode = TLV493D_OP_MODE_ULTRA_LOWPOWER;
		return 0;

	case 100:
		*mode = TLV493D_OP_MODE_LOWPOWER;
		return 0;

	case 3300:
		*mode = TLV493D_OP_MODE_FAST;
		return 0;

	default:
		return -EINVAL;
	}
}

static int tlv493d_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	ARG_UNUSED(chan);
	struct tlv493d_data *data = dev->data;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = tlv493d_get_mode_from_odr(val, &data->mode);

		if (ret < 0) {
			return ret;
		}

		ret = tlv493d_set_operating_mode(data->wr_regs, data->mode);

		if (ret < 0) {
			return ret;
		}

		return tlv493d_write_config(dev);
	default:
		return -ENOTSUP;
	}
}

#ifdef CONFIG_PM_DEVICE
static int tlv493d_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct tlv493d_data *data = dev->data;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = tlv493d_set_operating_mode(data->wr_regs, data->mode);

		if (ret < 0) {
			return ret;
		}

		return tlv493d_write_config(dev);
	case PM_DEVICE_ACTION_SUSPEND:
		ret = tlv493d_set_operating_mode(data->wr_regs, TLV493D_OP_MODE_POWERDOWN);

		if (ret < 0) {
			return ret;
		}

		return tlv493d_write_config(dev);
	default:
		return -ENOTSUP;
	}
}
#endif

static DEVICE_API(sensor, tlv493d_driver_api) = {
	.attr_set = tlv493d_attr_set,
	.attr_get = tlv493d_attr_get,
	.sample_fetch = tlv493d_sample_fetch,
	.channel_get = tlv493d_channel_get,
};

#define TLV493D_INST(inst)                                                                         \
	static struct tlv493d_data tlv493d_data_##inst;                                            \
	static const struct tlv493d_config tlv493d_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.mode = DT_INST_ENUM_IDX(inst, operating_mode),                                    \
		.temp_disable = DT_INST_PROP(inst, temp_disable),                                  \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, tlv493d_pm_action);                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tlv493d_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &tlv493d_data_##inst, &tlv493d_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &tlv493d_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLV493D_INST)
