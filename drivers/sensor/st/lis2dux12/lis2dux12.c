/* ST Microelectronics LIS2DUX12 3-axis accelerometer driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dux12.pdf
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>

#include "lis2dux12.h"

#if DT_HAS_COMPAT_STATUS_OKAY(st_lis2dux12)
#include "lis2dux12_api.h"
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(st_lis2duxs12)
#include "lis2duxs12_api.h"
#endif

LOG_MODULE_REGISTER(LIS2DUX12, CONFIG_SENSOR_LOG_LEVEL);

#define FOREACH_ODR_ENUM(ODR_VAL)			\
	ODR_VAL(LIS2DUX12_DT_ODR_OFF, 0.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_1Hz_ULP, 1.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_3Hz_ULP, 3.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_25Hz_ULP, 25.0f)	\
	ODR_VAL(LIS2DUX12_DT_ODR_6Hz, 6.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_12Hz5, 12.50f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_25Hz, 25.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_50Hz, 50.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_100Hz, 100.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_200Hz, 200.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_400Hz, 400.0f)		\
	ODR_VAL(LIS2DUX12_DT_ODR_800Hz, 800.0f)

#define GENERATE_VAL(ENUM, VAL) VAL,

static const float lis2dux12_odr_map[LIS2DUX12_DT_ODR_END] = {FOREACH_ODR_ENUM(GENERATE_VAL)};

static int lis2dux12_freq_to_odr_val(const struct device *dev, uint16_t freq)
{
	const struct lis2dux12_config *cfg = dev->config;

	/* constrain loop to prevent erroneous power mode/odr combinations */
	size_t i = (cfg->pm != LIS2DUX12_OPER_MODE_LOW_POWER) ? LIS2DUX12_DT_ODR_6Hz
							      : LIS2DUX12_DT_ODR_1Hz_ULP;
	size_t len = (cfg->pm != LIS2DUX12_OPER_MODE_LOW_POWER) ? LIS2DUX12_DT_ODR_END
								: LIS2DUX12_DT_ODR_6Hz;

	while (i < len) {
		if (freq <= lis2dux12_odr_map[i]) {
			return i;
		}
		++i;
	}

	return -EINVAL;
}

static int lis2dux12_set_fs(const struct device *dev, int16_t fs)
{
	int ret;
	uint8_t range;
	const struct lis2dux12_config *const cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;

	switch (fs) {
	case 2:
		range = LIS2DUX12_DT_FS_2G;
		break;
	case 4:
		range = LIS2DUX12_DT_FS_4G;
		break;
	case 8:
		range = LIS2DUX12_DT_FS_8G;
		break;
	case 16:
		range = LIS2DUX12_DT_FS_16G;
		break;
	default:
		LOG_ERR("fs [%d] not supported.", fs);
		return -EINVAL;
	}

	ret = chip_api->set_range(dev, range);
	if (ret < 0) {
		LOG_ERR("%s: range init error %d", dev->name, range);
		return ret;
	}

	LOG_DBG("%s: set fs to %d g", dev->name, fs);
	return ret;
}

static int lis2dux12_accel_config(const struct device *dev, enum sensor_channel chan,
				  enum sensor_attribute attr, const struct sensor_value *val)
{
	int odr_val;
	const struct lis2dux12_config *const cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2dux12_set_fs(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		odr_val = lis2dux12_freq_to_odr_val(dev, val->val1);
		if (odr_val < 0) {
			LOG_ERR("%d Hz not supported or wrong operating mode.", val->val1);
			return odr_val;
		}

		LOG_DBG("%s: set odr to %d Hz", dev->name, val->val1);

		return chip_api->set_odr_raw(dev, odr_val);
	default:
		LOG_ERR("Accel attribute not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2dux12_attr_set(const struct device *dev, enum sensor_channel chan,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2dux12_accel_config(dev, chan, attr, val);
	default:
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int lis2dux12_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct lis2dux12_config *const cfg = dev->config;
	const struct lis2dux12_chip_api *chip_api = cfg->chip_api;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = chip_api->sample_fetch_accel(dev);
		break;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		ret = chip_api->sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		ret = chip_api->sample_fetch_accel(dev);
		if (ret != 0)
			break;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
		ret = chip_api->sample_fetch_temp(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static inline void lis2dux12_convert(struct sensor_value *val, int raw_val, float gain)
{
	int64_t dval;

	/* Gain is in mg/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline int lis2dux12_get_channel(enum sensor_channel chan, struct sensor_value *val,
					struct lis2dux12_data *data, float gain)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lis2dux12_convert(val, data->sample_x, gain);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lis2dux12_convert(val, data->sample_y, gain);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lis2dux12_convert(val, data->sample_z, gain);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2dux12_convert(val, data->sample_x, gain);
		lis2dux12_convert(val + 1, data->sample_y, gain);
		lis2dux12_convert(val + 2, data->sample_z, gain);
		break;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		sensor_value_from_float(val, data->sample_temp);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lis2dux12_channel_get(const struct device *dev, enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct lis2dux12_data *data = dev->data;

	return lis2dux12_get_channel(chan, val, data, data->gain);
}

static DEVICE_API(sensor, lis2dux12_driver_api) = {
	.attr_set = lis2dux12_attr_set,
#if defined(CONFIG_LIS2DUX12_TRIGGER)
	.trigger_set = lis2dux12_trigger_set,
#endif
	.sample_fetch = lis2dux12_sample_fetch,
	.channel_get = lis2dux12_channel_get,
};

/*
 * Device creation macro, shared by LIS2DUX12_DEFINE_SPI() and
 * LIS2DUX12_DEFINE_I2C().
 */

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LIS2DUX12_TRIGGER
#define LIS2DUX12_CFG_IRQ(inst)								\
	.trig_enabled = true,								\
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),			\
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),			\
	.drdy_pin = DT_INST_PROP(inst, drdy_pin),
#else
#define LIS2DUX12_CFG_IRQ(inst)
#endif /* CONFIG_LIS2DUX12_TRIGGER */

#define LIS2DUX12_CONFIG_COMMON(inst, name)						\
	.chip_api = &name##_chip_api,							\
	.range = DT_INST_PROP(inst, range),						\
	.pm = DT_INST_PROP(inst, power_mode),						\
	.odr = DT_INST_PROP(inst, odr),							\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),			\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),			\
		   (LIS2DUX12_CFG_IRQ(inst)))						\

#define LIS2DUX12_SPI_OPERATION								\
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

/*
 * Instantiation macros used when a device is on a SPI bus.
 */
#define LIS2DUX12_CONFIG_SPI(inst, name)						\
	{										\
		STMEMSC_CTX_SPI(&lis2dux12_config_##name##_##inst.stmemsc_cfg),		\
		.stmemsc_cfg = {							\
			.spi = SPI_DT_SPEC_INST_GET(inst, LIS2DUX12_SPI_OPERATION, 0),	\
		},									\
		LIS2DUX12_CONFIG_COMMON(inst, name)					\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */
#define LIS2DUX12_CONFIG_I2C(inst, name)						\
	{										\
		STMEMSC_CTX_I2C(&lis2dux12_config_##name##_##inst.stmemsc_cfg),		\
		.stmemsc_cfg = {							\
			.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		},									\
		LIS2DUX12_CONFIG_COMMON(inst, name)					\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LIS2DUX12_DEFINE(inst, name)							\
	static struct lis2dux12_data lis2dux12_data_##name##_##inst;			\
	static const struct lis2dux12_config lis2dux12_config_##name##_##inst =		\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
			    (LIS2DUX12_CONFIG_SPI(inst, name)),				\
			    (LIS2DUX12_CONFIG_I2C(inst, name)));			\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, name##_init, NULL,				\
				     &lis2dux12_data_##name##_##inst,			\
				     &lis2dux12_config_##name##_##inst, POST_KERNEL,	\
				     CONFIG_SENSOR_INIT_PRIORITY,			\
				     &lis2dux12_driver_api);


#define DT_DRV_COMPAT st_lis2dux12
DT_INST_FOREACH_STATUS_OKAY_VARGS(LIS2DUX12_DEFINE, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_lis2duxs12
DT_INST_FOREACH_STATUS_OKAY_VARGS(LIS2DUX12_DEFINE, DT_DRV_COMPAT)
#undef DT_DRV_COMPAT
