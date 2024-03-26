/* ST Microelectronics LIS2DUX12 3-axis accelerometer driver
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dux12.pdf
 */

#define DT_DRV_COMPAT st_lis2dux12

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/sensor/lis2dux12.h>

#include "lis2dux12.h"

LOG_MODULE_REGISTER(LIS2DUX12, CONFIG_SENSOR_LOG_LEVEL);

static int lis2dux12_set_odr(const struct device *dev, uint8_t odr)
{
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_md_t mode = {.odr = odr};

	return lis2dux12_mode_set(ctx, &mode);
}

static int lis2dux12_set_range(const struct device *dev, uint8_t range)
{
	int err;
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_md_t val = { .odr = cfg->odr, .fs = range };

	err = lis2dux12_mode_set(ctx, &val);

	if (err) {
		return err;
	}

	switch (range) {
	default:
		LOG_ERR("range [%d] not supported.", range);
		return -EINVAL;
	case LIS2DUX12_DT_FS_2G:
		data->gain = lis2dux12_from_fs2g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_4G:
		data->gain = lis2dux12_from_fs4g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_8G:
		data->gain = lis2dux12_from_fs8g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_16G:
		data->gain = lis2dux12_from_fs16g_to_mg(1);
		break;
	}

	return 0;
}

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

static int lis2dux12_accel_config(const struct device *dev, enum sensor_channel chan,
				  enum sensor_attribute attr, const struct sensor_value *val)
{
	int odr_val;

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2dux12_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		odr_val = lis2dux12_freq_to_odr_val(dev, val->val1);
		if (odr_val < 0) {
			LOG_ERR("%d Hz not supported or wrong operating mode.", val->val1);
			return odr_val;
		}

		LOG_DBG("%s: set odr to %d Hz", dev->name, val->val1);

		return lis2dux12_set_odr(dev, odr_val);
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

static int lis2dux12_sample_fetch_accel(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	/* fetch raw data sample */
	lis2dux12_md_t mode = {.fs = cfg->range};
	lis2dux12_xl_data_t xzy_data = {0};

	if (lis2dux12_xl_data_get(ctx, &mode, &xzy_data) < 0) {
		LOG_ERR("Failed to fetch raw data sample");
		return -EIO;
	}

	data->sample_x = sys_le16_to_cpu(xzy_data.raw[0]);
	data->sample_y = sys_le16_to_cpu(xzy_data.raw[1]);
	data->sample_z = sys_le16_to_cpu(xzy_data.raw[2]);

	return 0;
}

#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
static int lis2dux12_sample_fetch_temp(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	/* fetch raw data sample */
	lis2dux12_md_t mode;
	lis2dux12_outt_data_t temp_data = {0};

	if (lis2dux12_outt_data_get(ctx, &mode, &temp_data) < 0) {
		LOG_ERR("Failed to fetch raw temperature data sample");
		return -EIO;
	}

	data->sample_temp = sys_le16_to_cpu(temp_data.heat.raw);

	return 0;
}
#endif

static int lis2dux12_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2dux12_sample_fetch_accel(dev);
		break;
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
	case SENSOR_CHAN_DIE_TEMP:
		lis2dux12_sample_fetch_temp(dev);
		break;
#endif
	case SENSOR_CHAN_ALL:
		lis2dux12_sample_fetch_accel(dev);
#if defined(CONFIG_LIS2DUX12_ENABLE_TEMP)
		lis2dux12_sample_fetch_temp(dev);
#endif
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
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
		sensor_value_from_double(val, data->sample_temp);
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

static const struct sensor_driver_api lis2dux12_driver_api = {
	.attr_set = lis2dux12_attr_set,
#if defined(CONFIG_LIS2DUX12_TRIGGER)
	.trigger_set = lis2dux12_trigger_set,
#endif
	.sample_fetch = lis2dux12_sample_fetch,
	.channel_get = lis2dux12_channel_get,
};

static int lis2dux12_init(const struct device *dev)
{
	const struct lis2dux12_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id;
	int ret;

	lis2dux12_exit_deep_power_down(ctx);
	k_busy_wait(25000);

	/* check chip ID */
	ret = lis2dux12_device_id_get(ctx, &chip_id);
	if (ret < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return ret;
	}

	if (chip_id != LIS2DUX12_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, chip_id);
		return -EINVAL;
	}

	/* reset device */
	ret = lis2dux12_init_set(ctx, LIS2DUX12_RESET);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(100);

	LOG_INF("%s: chip id 0x%x", dev->name, chip_id);

	/* Set bdu and if_inc recommended for driver usage */
	lis2dux12_init_set(ctx, LIS2DUX12_SENSOR_ONLY_ON);

	lis2dux12_timestamp_set(ctx, PROPERTY_ENABLE);

#ifdef CONFIG_LIS2DUX12_TRIGGER
	if (cfg->trig_enabled) {
		ret = lis2dux12_trigger_init(dev);
		if (ret < 0) {
			LOG_ERR("%s: Failed to initialize triggers", dev->name);
			return ret;
		}
	}
#endif

	/* set sensor default pm and odr */
	LOG_DBG("%s: pm: %d, odr: %d", dev->name, cfg->pm, cfg->odr);
	lis2dux12_md_t mode = {
		.odr = cfg->odr,
		.fs = cfg->range,
	};
	ret = lis2dux12_mode_set(ctx, &mode);
	if (ret < 0) {
		LOG_ERR("%s: odr init error (12.5 Hz)", dev->name);
		return ret;
	}

	/* set sensor default scale (used to convert sample values) */
	LOG_DBG("%s: range is %d", dev->name, cfg->range);
	ret = lis2dux12_set_range(dev, cfg->range);
	if (ret < 0) {
		LOG_ERR("%s: range init error %d", dev->name, cfg->range);
		return ret;
	}

	return 0;
}

/*
 * Device creation macro, shared by LIS2DUX12_DEFINE_SPI() and
 * LIS2DUX12_DEFINE_I2C().
 */

#define LIS2DUX12_DEVICE_INIT(inst)								\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lis2dux12_init, NULL, &lis2dux12_data_##inst,	\
				     &lis2dux12_config_##inst, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &lis2dux12_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LIS2DUX12_TRIGGER
#define LIS2DUX12_CFG_IRQ(inst)                                                                    \
	.trig_enabled = true,                                                                      \
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),                              \
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),                              \
	.drdy_pin = DT_INST_PROP(inst, drdy_pin),
#else
#define LIS2DUX12_CFG_IRQ(inst)
#endif /* CONFIG_LIS2DUX12_TRIGGER */

#define LIS2DUX12_SPI_OPERATION								\
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define LIS2DUX12_CONFIG_SPI(inst)							\
	{										\
		STMEMSC_CTX_SPI(&lis2dux12_config_##inst.stmemsc_cfg),			\
		.stmemsc_cfg = {							\
			.spi = SPI_DT_SPEC_INST_GET(inst, LIS2DUX12_SPI_OPERATION, 0),	\
		},									\
		.range = DT_INST_PROP(inst, range),					\
		.pm = DT_INST_PROP(inst, power_mode),					\
		.odr = DT_INST_PROP(inst, odr),						\
		IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),		\
				   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),		\
			   (LIS2DUX12_CFG_IRQ(inst)))					\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS2DUX12_CONFIG_I2C(inst)							\
	{										\
		STMEMSC_CTX_I2C(&lis2dux12_config_##inst.stmemsc_cfg),			\
		.stmemsc_cfg = {							\
			.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		},									\
		.range = DT_INST_PROP(inst, range),					\
		.pm = DT_INST_PROP(inst, power_mode),					\
		.odr = DT_INST_PROP(inst, odr),						\
		IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),		\
				   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),		\
			   (LIS2DUX12_CFG_IRQ(inst)))					\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LIS2DUX12_DEFINE(inst)								\
	static struct lis2dux12_data lis2dux12_data_##inst;				\
	static const struct lis2dux12_config lis2dux12_config_##inst =			\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (LIS2DUX12_CONFIG_SPI(inst)),	\
			    (LIS2DUX12_CONFIG_I2C(inst)));				\
	LIS2DUX12_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS2DUX12_DEFINE)
