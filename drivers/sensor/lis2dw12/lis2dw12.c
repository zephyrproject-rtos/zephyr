/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#define DT_DRV_COMPAT st_lis2dw12

#include <init.h>
#include <sys/__assert.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <drivers/i2c.h>
#endif

#include "lis2dw12.h"

LOG_MODULE_REGISTER(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2dw12_set_range - set full scale range for acc
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int lis2dw12_set_range(const struct device *dev, uint16_t range)
{
	int err;
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	uint8_t shift_gain = 0U;
	uint8_t fs = LIS2DW12_FS_TO_REG(range);

	err = lis2dw12_full_scale_set(lis2dw12->ctx, fs);

	if (cfg->pm == LIS2DW12_CONT_LOW_PWR_12bit) {
		shift_gain = LIS2DW12_SHFT_GAIN_NOLP1;
	}

	if (!err) {
		/* save internally gain for optimization */
		lis2dw12->gain =
			LIS2DW12_FS_TO_GAIN(LIS2DW12_FS_TO_REG(range),
					    shift_gain);
	}

	return err;
}

/**
 * lis2dw12_set_odr - set new sampling frequency
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @odr: Output data rate
 */
static int lis2dw12_set_odr(const struct device *dev, uint16_t odr)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	uint8_t val;

	/* check if power off */
	if (odr == 0U) {
		return lis2dw12_data_rate_set(lis2dw12->ctx,
					      LIS2DW12_XL_ODR_OFF);
	}

	val =  LIS2DW12_ODR_TO_REG(odr);
	if (val > LIS2DW12_XL_ODR_1k6Hz) {
		LOG_ERR("ODR too high");
		return -ENOTSUP;
	}

	return lis2dw12_data_rate_set(lis2dw12->ctx, val);
}

static inline void lis2dw12_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in ug/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void lis2dw12_channel_get_acc(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct lis2dw12_data *lis2dw12 = dev->data;
	struct sensor_value *pval = val;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		ofs_start = ofs_stop = 0U;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		ofs_start = ofs_stop = 1U;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		ofs_start = ofs_stop = 2U;
		break;
	default:
		ofs_start = 0U; ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop ; i++) {
		lis2dw12_convert(pval++, lis2dw12->acc[i], lis2dw12->gain);
	}
}

static int lis2dw12_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		lis2dw12_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

static int lis2dw12_config(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lis2dw12_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2dw12_set_odr(dev, val->val1);
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int lis2dw12_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return lis2dw12_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int lis2dw12_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	uint8_t shift;
	int16_t buf[3];

	/* fetch raw data sample */
	if (lis2dw12_acceleration_raw_get(lis2dw12->ctx, buf) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	/* adjust to resolution */
	if (cfg->pm == LIS2DW12_CONT_LOW_PWR_12bit) {
		shift = LIS2DW12_SHIFT_PM1;
	} else {
		shift = LIS2DW12_SHIFT_PMOTHER;
	}

	lis2dw12->acc[0] = sys_le16_to_cpu(buf[0]) >> shift;
	lis2dw12->acc[1] = sys_le16_to_cpu(buf[1]) >> shift;
	lis2dw12->acc[2] = sys_le16_to_cpu(buf[2]) >> shift;

	return 0;
}

static const struct sensor_driver_api lis2dw12_driver_api = {
	.attr_set = lis2dw12_attr_set,
#if CONFIG_LIS2DW12_TRIGGER
	.trigger_set = lis2dw12_trigger_set,
#endif /* CONFIG_LIS2DW12_TRIGGER */
	.sample_fetch = lis2dw12_sample_fetch,
	.channel_get = lis2dw12_channel_get,
};

static int lis2dw12_init_interface(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;

	lis2dw12->bus = device_get_binding(cfg->bus_name);
	if (!lis2dw12->bus) {
		LOG_DBG("master bus not found: %s", cfg->bus_name);
		return -EINVAL;
	}

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	lis2dw12_spi_init(dev);
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	lis2dw12_i2c_init(dev);
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

	return 0;
}

static int lis2dw12_set_power_mode(struct lis2dw12_data *lis2dw12,
				    lis2dw12_mode_t pm)
{
	uint8_t regval = LIS2DW12_CONT_LOW_PWR_12bit;

	switch (pm) {
	case LIS2DW12_CONT_LOW_PWR_2:
	case LIS2DW12_CONT_LOW_PWR_3:
	case LIS2DW12_CONT_LOW_PWR_4:
	case LIS2DW12_HIGH_PERFORMANCE:
		regval = pm;
		break;
	default:
		LOG_DBG("Apply default Power Mode");
		break;
	}

	return lis2dw12_write_reg(lis2dw12->ctx, LIS2DW12_CTRL1, &regval, 1);
}

static int lis2dw12_init(const struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	uint8_t wai;

	if (lis2dw12_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (lis2dw12_device_id_get(lis2dw12->ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != LIS2DW12_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	if (lis2dw12_reset_set(lis2dw12->ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	k_busy_wait(100);

	if (lis2dw12_block_data_update_set(lis2dw12->ctx,
					   PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	/* set power mode */
	if (lis2dw12_set_power_mode(lis2dw12, CONFIG_LIS2DW12_POWER_MODE)) {
		return -EIO;
	}

	/* set default odr and full scale for acc */
	if (lis2dw12_data_rate_set(lis2dw12->ctx, LIS2DW12_DEFAULT_ODR) < 0) {
		return -EIO;
	}

	if (lis2dw12_full_scale_set(lis2dw12->ctx, LIS2DW12_ACC_FS) < 0) {
		return -EIO;
	}

	lis2dw12->gain =
		LIS2DW12_FS_TO_GAIN(LIS2DW12_ACC_FS,
				    cfg->pm == LIS2DW12_CONT_LOW_PWR_12bit ?
				    LIS2DW12_SHFT_GAIN_NOLP1 : 0);

#ifdef CONFIG_LIS2DW12_TRIGGER
	if (lis2dw12_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}
#endif /* CONFIG_LIS2DW12_TRIGGER */

	return 0;
}

const struct lis2dw12_device_config lis2dw12_cfg = {
	.bus_name = DT_INST_BUS_LABEL(0),
	.pm = CONFIG_LIS2DW12_POWER_MODE,
#ifdef CONFIG_LIS2DW12_TRIGGER
	.gpio_int = GPIO_DT_SPEC_INST_GET_OR(0, irq_gpios, {0}),
	.int_pin = DT_INST_PROP(0, int_pin),

#ifdef CONFIG_LIS2DW12_TAP
	.tap_mode = DT_INST_PROP(0, tap_mode),
	.tap_threshold = DT_INST_PROP(0, tap_threshold),
	.tap_shock = DT_INST_PROP(0, tap_shock),
	.tap_latency = DT_INST_PROP(0, tap_latency),
	.tap_quiet = DT_INST_PROP(0, tap_quiet),
#endif /* CONFIG_LIS2DW12_TAP */
#endif /* CONFIG_LIS2DW12_TRIGGER */
};

struct lis2dw12_data lis2dw12_data;

DEVICE_DT_INST_DEFINE(0, lis2dw12_init, NULL,
	     &lis2dw12_data, &lis2dw12_cfg, POST_KERNEL,
	     CONFIG_SENSOR_INIT_PRIORITY, &lis2dw12_driver_api);
