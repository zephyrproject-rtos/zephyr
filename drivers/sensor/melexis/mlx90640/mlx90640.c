/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT melexis_mlx90640

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mlx90640.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "mlx90640.h"

LOG_MODULE_REGISTER(MLX90640, CONFIG_SENSOR_LOG_LEVEL);

#define MLX90640_I2C_CHUNK_WORDS 32

static const uint16_t mlx90640_hz_milli[] = {
	500, 1000, 2000, 4000, 8000, 16000, 32000, 64000,
};

static int mlx90640_read_words(const struct i2c_dt_spec *i2c, uint16_t start, uint16_t *words,
			       size_t count)
{
	while (count > 0) {
		uint8_t addr[2];
		uint8_t buf[MLX90640_I2C_CHUNK_WORDS * 2];
		size_t n = MIN(count, (size_t)MLX90640_I2C_CHUNK_WORDS);
		int ret;

		sys_put_be16(start, addr);
		ret = i2c_write_read_dt(i2c, addr, sizeof(addr), buf, n * 2U);
		if (ret < 0) {
			return ret;
		}

		for (size_t i = 0; i < n; i++) {
			words[i] = sys_get_be16(&buf[i * 2U]);
		}

		words += n;
		start += (uint16_t)n;
		count -= n;
	}

	return 0;
}

static int mlx90640_write_word(const struct i2c_dt_spec *i2c, uint16_t reg, uint16_t value)
{
	uint8_t buf[4];

	sys_put_be16(reg, &buf[0]);
	sys_put_be16(value, &buf[2]);

	return i2c_write_dt(i2c, buf, sizeof(buf));
}

static uint32_t mlx90640_subpage_timeout_ms(uint8_t rate)
{
	/* One subpage period is 1 / refresh; wait up to 3 periods. */
	uint32_t period_ms = 2000U >> MIN(rate, 7);

	if (period_ms == 0U) {
		period_ms = 16U;
	}

	return period_ms * 3U;
}

static int mlx90640_get_subpage(const struct device *dev)
{
	const struct mlx90640_config *cfg = dev->config;
	struct mlx90640_data *data = dev->data;
	uint32_t timeout = mlx90640_subpage_timeout_ms(data->refresh_rate);
	uint32_t start = k_uptime_get_32();
	uint16_t status;
	uint16_t control;
	int ret;
	int tries = 0;

	while (true) {
		ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}

		if ((status & MLX90640_STATUS_NEW_DATA) != 0U) {
			break;
		}

		if ((k_uptime_get_32() - start) > timeout) {
			LOG_ERR("timed out waiting for RAM subpage");
			return -EAGAIN;
		}

		k_msleep(5);
	}

	while (((status & MLX90640_STATUS_NEW_DATA) != 0U) && (tries < 5)) {
		ret = mlx90640_write_word(&cfg->i2c, MLX90640_REG_STATUS, MLX90640_STATUS_CLEAR);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_RAM, data->frame,
					  MLX90640_RAM_WORDS);
		if (ret < 0) {
			return ret;
		}

		ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_STATUS, &status, 1);
		if (ret < 0) {
			return ret;
		}

		tries++;
	}

	if ((status & MLX90640_STATUS_NEW_DATA) != 0U) {
		LOG_ERR("RAM still busy after %d reads", tries);
		return -EIO;
	}

	ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_CONTROL, &control, 1);
	if (ret < 0) {
		return ret;
	}

	data->frame[832] = control;
	data->frame[833] = status & MLX90640_STATUS_SUBPAGE;

	return 0;
}

static int mlx90640_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mlx90640_data *data = dev->data;
	const struct mlx90640_config *cfg = dev->config;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP) &&
	    (chan != SENSOR_CHAN_DIE_TEMP)) {
		return -ENOTSUP;
	}

	for (int i = 0; i < 2; i++) {
		float tr;

		ret = mlx90640_get_subpage(dev);
		if (ret < 0) {
			return ret;
		}

		data->ta = mlx90640_calc_ta(&data->params, data->frame);
		tr = data->ta - (float)cfg->ta_shift;
		mlx90640_calc_to(&data->params, data->frame, data->emissivity, tr, data->to);
	}

	return 0;
}

static void mlx90640_float_to_sensor_value(float temp, struct sensor_value *val)
{
	int32_t micro = (int32_t)(temp * 1000000.0f);

	val->val1 = micro / 1000000;
	val->val2 = micro % 1000000;
	if ((val->val2 < 0) && (val->val1 > 0)) {
		val->val1--;
		val->val2 += 1000000;
	} else if ((val->val2 > 0) && (val->val1 < 0)) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

static int mlx90640_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct mlx90640_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		for (int i = 0; i < MLX90640_PIXELS; i++) {
			mlx90640_float_to_sensor_value(data->to[i], &val[i]);
		}
		return 0;
	case SENSOR_CHAN_DIE_TEMP:
		mlx90640_float_to_sensor_value(data->ta, val);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mlx90640_refresh_from_hz(const struct sensor_value *val, uint8_t *rate)
{
	int32_t milli = val->val1 * 1000 + val->val2 / 1000;

	if (milli < 0) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(mlx90640_hz_milli); i++) {
		if (milli <= (int32_t)mlx90640_hz_milli[i] + (int32_t)mlx90640_hz_milli[i] / 4) {
			*rate = i;
			return 0;
		}
	}

	*rate = 7;
	return 0;
}

static int mlx90640_set_refresh(const struct device *dev, uint8_t rate)
{
	const struct mlx90640_config *cfg = dev->config;
	uint16_t control;
	int ret;

	if (rate > 7U) {
		return -EINVAL;
	}

	ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_CONTROL, &control, 1);
	if (ret < 0) {
		return ret;
	}

	control &= ~MLX90640_CTRL_REFRESH_MASK;
	control |= FIELD_PREP(MLX90640_CTRL_REFRESH_MASK, rate);

	return mlx90640_write_word(&cfg->i2c, MLX90640_REG_CONTROL, control);
}

static int mlx90640_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct mlx90640_data *data = dev->data;
	uint8_t rate;

	ARG_UNUSED(chan);

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		int ret = mlx90640_refresh_from_hz(val, &rate);

		if (ret < 0) {
			return ret;
		}

		ret = mlx90640_set_refresh(dev, rate);
		if (ret == 0) {
			data->refresh_rate = rate;
		}
		return ret;
	}

	if ((unsigned int)attr == MLX90640_SENSOR_ATTR_EMISSIVITY) {
		float e = (float)val->val1 + (float)val->val2 / 1000000.0f;

		if ((e <= 0.0f) || (e > 1.0f)) {
			return -EINVAL;
		}

		data->emissivity = e;
		return 0;
	}

	return -ENOTSUP;
}

static int mlx90640_attr_get(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct mlx90640_data *data = dev->data;

	ARG_UNUSED(chan);

	if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		uint16_t milli = mlx90640_hz_milli[MIN(data->refresh_rate, 7)];

		val->val1 = milli / 1000;
		val->val2 = (milli % 1000) * 1000;
		return 0;
	}

	if ((unsigned int)attr == MLX90640_SENSOR_ATTR_EMISSIVITY) {
		mlx90640_float_to_sensor_value(data->emissivity, val);
		return 0;
	}

	return -ENOTSUP;
}

static int mlx90640_configure(const struct device *dev)
{
	const struct mlx90640_config *cfg = dev->config;
	uint16_t control;
	int ret;

	ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_CONTROL, &control, 1);
	if (ret < 0) {
		return ret;
	}

	control |= MLX90640_CTRL_CHESS | MLX90640_CTRL_SUBPAGES;
	control &= ~MLX90640_CTRL_REFRESH_MASK;
	control |= FIELD_PREP(MLX90640_CTRL_REFRESH_MASK, cfg->refresh_rate);

	return mlx90640_write_word(&cfg->i2c, MLX90640_REG_CONTROL, control);
}

static int mlx90640_init(const struct device *dev)
{
	const struct mlx90640_config *cfg = dev->config;
	struct mlx90640_data *data = dev->data;
	uint16_t id[3];
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_REGULATOR) && (cfg->vin_supply != NULL)) {
		if (!device_is_ready(cfg->vin_supply)) {
			LOG_ERR("vin-supply not ready");
			return -ENODEV;
		}

		ret = regulator_enable(cfg->vin_supply);
		if (ret < 0) {
			LOG_ERR("failed to enable vin-supply (%d)", ret);
			return ret;
		}

		k_msleep(80);
	}

	ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_DEVICE_ID, id, ARRAY_SIZE(id));
	if (ret < 0) {
		LOG_ERR("failed to read device id (%d)", ret);
		return ret;
	}

	if (((id[0] | id[1] | id[2]) == 0U) || ((id[0] & id[1] & id[2]) == 0xFFFFU)) {
		LOG_ERR("invalid device id %04x %04x %04x", id[0], id[1], id[2]);
		return -ENODEV;
	}

	LOG_DBG("device id %04x %04x %04x", id[0], id[1], id[2]);

	ret = mlx90640_read_words(&cfg->i2c, MLX90640_REG_EEPROM, data->frame,
				  MLX90640_EEPROM_WORDS);
	if (ret < 0) {
		LOG_ERR("failed to dump EEPROM (%d)", ret);
		return ret;
	}

	ret = mlx90640_extract_params(&data->params, data->frame);
	if (ret < 0) {
		LOG_ERR("invalid EEPROM calibration data");
		return ret;
	}

	if ((data->params.broken_count > 0U) || (data->params.outlier_count > 0U)) {
		LOG_WRN("%u broken / %u outlier pixels", data->params.broken_count,
			data->params.outlier_count);
	}

	data->refresh_rate = cfg->refresh_rate;
	data->emissivity = (float)cfg->emissivity / 1000000.0f;
	if ((data->emissivity <= 0.0f) || (data->emissivity > 1.0f)) {
		data->emissivity = 0.95f;
	}

	ret = mlx90640_configure(dev);
	if (ret < 0) {
		LOG_ERR("failed to configure control register (%d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, mlx90640_api) = {
	.sample_fetch = mlx90640_sample_fetch,
	.channel_get = mlx90640_channel_get,
	.attr_set = mlx90640_attr_set,
	.attr_get = mlx90640_attr_get,
};

#define MLX90640_VIN_SUPPLY(inst)                                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, vin_supply),                                       \
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, vin_supply))), (NULL))

#define MLX90640_DEFINE(inst)                                                                      \
	static struct mlx90640_data mlx90640_data_##inst;                                          \
                                                                                                   \
	static const struct mlx90640_config mlx90640_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.vin_supply = MLX90640_VIN_SUPPLY(inst),                                           \
		.refresh_rate = DT_INST_PROP(inst, refresh_rate),                                  \
		.emissivity = DT_INST_PROP(inst, emissivity),                                      \
		.ta_shift = DT_INST_PROP(inst, ta_shift),                                          \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mlx90640_init, NULL, &mlx90640_data_##inst,             \
				     &mlx90640_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &mlx90640_api);

DT_INST_FOREACH_STATUS_OKAY(MLX90640_DEFINE)
