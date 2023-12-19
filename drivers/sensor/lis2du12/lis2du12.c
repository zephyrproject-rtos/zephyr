/* ST Microelectronics LIS2DU12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2du12.pdf
 */

#define DT_DRV_COMPAT st_lis2du12

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lis2du12.h"

LOG_MODULE_REGISTER(LIS2DU12, CONFIG_SENSOR_LOG_LEVEL);

static const float lis2du12_odr_map[14] = {
			0.0f, 1.6f, 3.0f, 6.0f, 6.0f, 12.5f, 25.0f,
			50.0f, 100.0f, 200.0f, 400.0f, 800.0f, 0.0f, 0.0f};

static int lis2du12_freq_to_odr_val(const struct device *dev, uint16_t freq)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2du12_odr_map); i++) {
		if (freq <= lis2du12_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static const uint16_t lis2du12_accel_fs_map[] = {2, 4, 8, 16};

static int lis2du12_accel_range_to_fs_val(int32_t range)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(lis2du12_accel_fs_map); i++) {
		if (range == lis2du12_accel_fs_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static inline int lis2du12_reboot(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_status_t status;
	uint8_t tries = 10;

	if (lis2du12_init_set(ctx, LIS2DU12_RESET) < 0) {
		return -EIO;
	}

	do {
		if (!--tries) {
			LOG_ERR("sw reset timed out");
			return -ETIMEDOUT;
		}
		k_usleep(50);

		if (lis2du12_status_get(ctx, &status) < 0) {
			return -EIO;
		}
	} while (status.sw_reset != 0);

	if (lis2du12_init_set(ctx, LIS2DU12_DRV_RDY) < 0) {
		return -EIO;
	}

	return 0;
}

static int lis2du12_accel_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lis2du12_data *data = dev->data;
	lis2du12_md_t mode;

	if (lis2du12_mode_get(ctx, &mode) < 0) {
		return -EIO;
	}

	mode.fs = fs;
	if (lis2du12_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

	data->accel_fs = fs;

	return 0;
}

static int lis2du12_accel_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lis2du12_data *data = dev->data;
	lis2du12_md_t mode;

	if (lis2du12_mode_get(ctx, &mode) < 0) {
		return -EIO;
	}

	mode.odr = odr;
	if (lis2du12_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

	data->accel_freq = odr;

	return 0;
}

static int lis2du12_accel_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	odr = lis2du12_freq_to_odr_val(dev, freq);
	if (odr < 0) {
		return odr;
	}

	if (lis2du12_accel_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static int lis2du12_accel_range_set(const struct device *dev, int32_t range)
{
	int fs;
	struct lis2du12_data *data = dev->data;

	fs = lis2du12_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (lis2du12_accel_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->acc_gain = lis2du12_accel_fs_map[fs] * GAIN_UNIT_XL / 2;
	return 0;
}

static int lis2du12_accel_config(const struct device *dev,
				 enum sensor_channel chan,
				 enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2du12_accel_range_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2du12_accel_odr_set(dev, val->val1);
	default:
		LOG_WRN("Accel attribute %d not supported.", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int lis2du12_attr_set(const struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2du12_accel_config(dev, chan, attr, val);
	default:
		LOG_WRN("attribute %d not supported on this channel.", chan);
		return -ENOTSUP;
	}

	return 0;
}

static int lis2du12_sample_fetch_accel(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lis2du12_data *data = dev->data;
	lis2du12_data_t xl_data;
	lis2du12_md_t md;

	md.fs = cfg->accel_range;
	if (lis2du12_data_get(ctx, &md, &xl_data) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->acc[0] = xl_data.xl.raw[0];
	data->acc[1] = xl_data.xl.raw[1];
	data->acc[2] = xl_data.xl.raw[2];

	return 0;
}

static int lis2du12_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2du12_sample_fetch_accel(dev);
		break;
	case SENSOR_CHAN_ALL:
		lis2du12_sample_fetch_accel(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static inline void lis2du12_accel_convert(struct sensor_value *val, int raw_val,
					  uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)(raw_val) * sensitivity * SENSOR_G_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);

}

static inline int lis2du12_accel_get_channel(enum sensor_channel chan,
					     struct sensor_value *val,
					     struct lis2du12_data *data,
					     uint32_t sensitivity)
{
	uint8_t i;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		lis2du12_accel_convert(val, data->acc[0], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		lis2du12_accel_convert(val, data->acc[1], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		lis2du12_accel_convert(val, data->acc[2], sensitivity);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		for (i = 0; i < 3; i++) {
			lis2du12_accel_convert(val++, data->acc[i], sensitivity);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int lis2du12_accel_channel_get(enum sensor_channel chan,
				      struct sensor_value *val,
				      struct lis2du12_data *data)
{
	return lis2du12_accel_get_channel(chan, val, data, data->acc_gain);
}

static int lis2du12_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct lis2du12_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2du12_accel_channel_get(chan, val, data);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lis2du12_driver_api = {
	.attr_set = lis2du12_attr_set,
#if CONFIG_LIS2DU12_TRIGGER
	.trigger_set = lis2du12_trigger_set,
#endif
	.sample_fetch = lis2du12_sample_fetch,
	.channel_get = lis2du12_channel_get,
};

static int lis2du12_init_chip(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lis2du12_data *lis2du12 = dev->data;
	lis2du12_id_t chip_id;
	uint8_t odr, fs;

	if (lis2du12_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id.whoami);

	if (chip_id.whoami != LIS2DU12_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id.whoami);
		return -EIO;
	}

	/* reboot device */
	if (lis2du12_reboot(dev) < 0) {
		return -EIO;
	}

	/* set FS from DT */
	fs = cfg->accel_range;
	LOG_DBG("accel range is %d", fs);
	if (lis2du12_accel_set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer range %d", fs);
		return -EIO;
	}
	lis2du12->acc_gain = lis2du12_accel_fs_map[fs] * GAIN_UNIT_XL / 2;

	/* set odr from DT (the only way to go in high performance) */
	odr = cfg->accel_odr;
	LOG_DBG("accel odr is %d", odr);
	if (lis2du12_accel_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer odr %d", odr);
		return -EIO;
	}

	return 0;
}

static int lis2du12_init(const struct device *dev)
{
#ifdef CONFIG_LIS2DU12_TRIGGER
	const struct lis2du12_config *cfg = dev->config;
#endif
	struct lis2du12_data *data = dev->data;

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;

	if (lis2du12_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LIS2DU12_TRIGGER
	if (cfg->trig_enabled) {
		if (lis2du12_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}

/*
 * Device creation macro, shared by LIS2DU12_DEFINE_SPI() and
 * LIS2DU12_DEFINE_I2C().
 */

#define LIS2DU12_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    lis2du12_init,				\
			    NULL,					\
			    &lis2du12_data_##inst,			\
			    &lis2du12_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lis2du12_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LIS2DU12_TRIGGER
#define LIS2DU12_CFG_IRQ(inst)						\
	.trig_enabled = true,						\
	.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, { 0 }), \
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, { 0 }), \
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),			\
	.drdy_pin = DT_INST_PROP(inst, drdy_pin)
#else
#define LIS2DU12_CFG_IRQ(inst)
#endif /* CONFIG_LIS2DU12_TRIGGER */

#define LIS2DU12_SPI_OP  (SPI_WORD_SET(8) |				\
			 SPI_OP_MODE_MASTER |				\
			 SPI_MODE_CPOL |				\
			 SPI_MODE_CPHA)					\

#define LIS2DU12_CONFIG_COMMON(inst)					\
	.accel_odr = DT_INST_PROP(inst, accel_odr),			\
	.accel_range = DT_INST_PROP(inst, accel_range),			\
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
		   (LIS2DU12_CFG_IRQ(inst)))

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define LIS2DU12_CONFIG_SPI(inst)						\
	{									\
		STMEMSC_CTX_SPI(&lis2du12_config_##inst.stmemsc_cfg),		\
		.stmemsc_cfg = {						\
			.spi = SPI_DT_SPEC_INST_GET(inst,			\
					   LIS2DU12_SPI_OP,			\
					   0),					\
		},								\
		LIS2DU12_CONFIG_COMMON(inst)					\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS2DU12_CONFIG_I2C(inst)						\
	{									\
		STMEMSC_CTX_I2C(&lis2du12_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {						\
			.i2c = I2C_DT_SPEC_INST_GET(inst),			\
		},								\
		LIS2DU12_CONFIG_COMMON(inst)					\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LIS2DU12_DEFINE(inst)						\
	static struct lis2du12_data lis2du12_data_##inst;			\
	static const struct lis2du12_config lis2du12_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			(LIS2DU12_CONFIG_SPI(inst)),			\
			(LIS2DU12_CONFIG_I2C(inst)));			\
	LIS2DU12_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS2DU12_DEFINE)
