/* ST Microelectronics LIS2DU12 / MIS2DU12 3-axis accelerometer sensor driver
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2025 8tronix GmbH
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheets:
 * https://www.st.com/resource/en/datasheet/lis2du12.pdf
 * https://www.st.com/resource/en/datasheet/mis2du12.pdf
 */

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

static const float lis2du12_odr_map[14] = {0.0f,  1.6f,   3.0f,   6.0f,   6.0f,   12.5f, 25.0f,
					   50.0f, 100.0f, 200.0f, 400.0f, 800.0f, 0.0f,  0.0f};

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

static int lis2du12_accel_odr_set(const struct device *dev, uint16_t freq)
{
	const struct lis2du12_config *cfg = dev->config;
	int odr;

	odr = lis2du12_freq_to_odr_val(dev, freq);
	if (odr < 0) {
		return odr;
	}

	if (cfg->chip_api->set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set accelerometer sampling rate");
		return -EIO;
	}

	return 0;
}

static int lis2du12_accel_range_set(const struct device *dev, int32_t range)
{
	const struct lis2du12_config *cfg = dev->config;
	struct lis2du12_data *data = dev->data;
	int fs;

	fs = lis2du12_accel_range_to_fs_val(range);
	if (fs < 0) {
		return fs;
	}

	if (cfg->chip_api->set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer full-scale");
		return -EIO;
	}

	data->accel_fs = lis2du12_accel_fs_map[fs];
	data->acc_gain = lis2du12_accel_fs_map[fs] * GAIN_UNIT_XL / 2;
	return 0;
}

static int lis2du12_accel_config(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2du12_accel_range_set(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2du12_accel_odr_set(dev, val->val1);
#if defined(CONFIG_LIS2DU12_TRIGGER)
	case SENSOR_ATTR_SLOPE_TH:
		return lis2du12_accel_set_wake_th(dev, val);
	case SENSOR_ATTR_SLOPE_DUR:
		return lis2du12_accel_set_wake_dur(dev, val);
#endif
	default:
		LOG_WRN("Accel attribute %d not supported.", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int lis2du12_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
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
	struct lis2du12_data *data = dev->data;

	return cfg->chip_api->read_raw(dev, data->acc);
}

static int lis2du12_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ALL:
		ret = lis2du12_sample_fetch_accel(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

static inline void lis2du12_accel_convert(struct sensor_value *val, int raw_val,
					  uint32_t sensitivity)
{
	int64_t dval;

	/* Sensitivity is exposed in ug/LSB */
	/* Convert to m/s^2 */
	dval = (int64_t)(raw_val)*sensitivity * SENSOR_G_DOUBLE;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);
}

static inline int lis2du12_accel_get_channel(enum sensor_channel chan, struct sensor_value *val,
					     struct lis2du12_data *data, uint32_t sensitivity)
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

static int lis2du12_accel_channel_get(enum sensor_channel chan, struct sensor_value *val,
				      struct lis2du12_data *data)
{
	return lis2du12_accel_get_channel(chan, val, data, data->acc_gain);
}

static int lis2du12_channel_get(const struct device *dev, enum sensor_channel chan,
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

static DEVICE_API(sensor, lis2du12_driver_api) = {
	.attr_set = lis2du12_attr_set,
#if CONFIG_LIS2DU12_TRIGGER
	.trigger_set = lis2du12_trigger_set,
#endif
	.sample_fetch = lis2du12_sample_fetch,
	.channel_get = lis2du12_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = lis2du12_submit,
	.get_decoder = lis2du12_get_decoder,
#endif
};

static int lis2du12_init_chip(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	struct lis2du12_data *lis2du12 = dev->data;
	uint8_t odr, fs;

	if (cfg->chip_api->check_id(dev) < 0) {
		return -EIO;
	}

	/* reboot device */
	if (cfg->chip_api->reboot(dev) < 0) {
		return -EIO;
	}

	/* set FS from DT */
	fs = cfg->accel_range;
	LOG_DBG("accel range is %d", fs);
	if (cfg->chip_api->set_fs_raw(dev, fs) < 0) {
		LOG_ERR("failed to set accelerometer range %d", fs);
		return -EIO;
	}
	lis2du12->accel_fs = lis2du12_accel_fs_map[fs];
	lis2du12->acc_gain = lis2du12_accel_fs_map[fs] * GAIN_UNIT_XL / 2;

	/* set odr from DT (the only way to go in high performance) */
	odr = cfg->accel_odr;
	LOG_DBG("accel odr is %d", odr);
	if (cfg->chip_api->set_odr_raw(dev, odr) < 0) {
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
 * Device creation macro, shared by SPI and I2C configs.
 */
#define LIS2DU12_DEVICE_INIT(orig, inst)                                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lis2du12_init, NULL, &orig##_data_##inst,               \
				     &orig##_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &lis2du12_driver_api);

#ifdef CONFIG_LIS2DU12_TRIGGER
#define LIS2DU12_CFG_IRQ(inst)                                                                     \
	.trig_enabled = true, .int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),        \
	.int2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),                              \
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed), .drdy_pin = DT_INST_PROP(inst, drdy_pin),  \
	.delta_pin = DT_INST_PROP(inst, delta_pin),
#else
#define LIS2DU12_CFG_IRQ(inst)
#endif /* CONFIG_LIS2DU12_TRIGGER */

#define LIS2DU12_SPI_OP (SPI_WORD_SET(8) | SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define LIS2DU12_CONFIG_COMMON(orig, type, inst)                                                   \
	.chip_api = &orig##_api, .chip_type = type, .accel_odr = DT_INST_PROP(inst, accel_odr),    \
	.accel_range = DT_INST_PROP(inst, accel_range),                                            \
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
		   (LIS2DU12_CFG_IRQ(inst)))

#define LIS2DU12_CONFIG_SPI(orig, type, inst)                                                      \
	{STMEMSC_CTX_SPI(&orig##_config_##inst.stmemsc_cfg),                                       \
	 .stmemsc_cfg = {.spi = SPI_DT_SPEC_INST_GET(inst, LIS2DU12_SPI_OP)},                      \
	 LIS2DU12_CONFIG_COMMON(orig, type, inst)}

#define LIS2DU12_CONFIG_I2C(orig, type, inst)                                                      \
	{STMEMSC_CTX_I2C(&orig##_config_##inst.stmemsc_cfg),                                       \
	 .stmemsc_cfg =                                                                            \
		 {                                                                                 \
			 .i2c = I2C_DT_SPEC_INST_GET(inst),                                        \
		 },                                                                                \
	 LIS2DU12_CONFIG_COMMON(orig, type, inst)}

/* clang-format off */
#define LIS2DU12_DEFINE(orig, type, inst)                                                          \
	static struct lis2du12_data orig##_data_##inst;                                            \
	static const struct lis2du12_config orig##_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			(LIS2DU12_CONFIG_SPI(orig, type, inst)),	\
			(LIS2DU12_CONFIG_I2C(orig, type, inst)));      \
	LIS2DU12_DEVICE_INIT(orig, inst)
/* clang-format on */

/*
 * Instantiate st,lis2du12 devices
 */
#define DT_DRV_COMPAT              st_lis2du12
#define LIS2DU12_INST_DEFINE(inst) LIS2DU12_DEFINE(lis2du12, LIS2DU12_CHIP_LIS2DU12, inst)
DT_INST_FOREACH_STATUS_OKAY(LIS2DU12_INST_DEFINE)

/*
 * Instantiate st,mis2du12 devices
 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT              st_mis2du12
#define MIS2DU12_INST_DEFINE(inst) LIS2DU12_DEFINE(mis2du12, LIS2DU12_CHIP_MIS2DU12, inst)
DT_INST_FOREACH_STATUS_OKAY(MIS2DU12_INST_DEFINE)
