/* ST Microelectronics IIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dh.pdf
 */

#define DT_DRV_COMPAT st_iis2dh

#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif

#include "iis2dh.h"

LOG_MODULE_REGISTER(IIS2DH, CONFIG_SENSOR_LOG_LEVEL);

/* gains in uG/LSB */
static const uint32_t iis2dh_gain[3][4] = {
	{
		/* HR mode */
		980/16,		/* 2G */
		1950/16,	/* 4G */
		3910/16,	/* 8G */
		11720/16,	/* 16G */
	},
	{
		/* NM mode */
		3910/64,	/* 2G */
		7810/64,	/* 4G */
		15630/64,	/* 8G */
		46950/64,	/* 16G */
	},
	{
		/* LP mode */
		15630/256,	/* 2G */
		31250/256,	/* 4G */
		62500/256,	/* 8G */
		188680/256,	/* 16G */
	},
};

static int iis2dh_set_fs_raw(const struct device *dev, uint8_t fs)
{
	struct iis2dh_data *iis2dh = dev->data;
	int err;

	err = iis2dh_full_scale_set(iis2dh->ctx, fs);

	if (!err) {
		/* save internally gain for optimization */
		iis2dh->gain = iis2dh_gain[IIS2DH_HR_12bit][fs];
	}

	return err;
}

#if (CONFIG_IIS2DH_RANGE == 0)
/**
 * iis2dh_set_range - set full scale range for acc
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int iis2dh_set_range(const struct device *dev, uint16_t range)
{
	int err;
	uint8_t fs = IIS2DH_FS_TO_REG(range);

	err = iis2dh_set_fs_raw(dev, fs);

	return err;
}
#endif

#if (CONFIG_IIS2DH_ODR == 0)
/**
 * iis2dh_set_odr - set new sampling frequency
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @odr: Output data rate
 */
static int iis2dh_set_odr(const struct device *dev, uint16_t odr)
{
	struct iis2dh_data *iis2dh = dev->data;
	const struct iis2dh_device_config *cfg = dev->config;
	iis2dh_odr_t val;

	val = IIS2DH_ODR_TO_REG_HR(cfg->pm, odr);

	return iis2dh_data_rate_set(iis2dh->ctx, val);
}
#endif

static inline void iis2dh_convert(struct sensor_value *val, int raw_val,
				  uint32_t gain)
{
	int64_t dval;

	/* Gain is in ug/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void iis2dh_channel_get_acc(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis2dh_data *iis2dh = dev->data;
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
		iis2dh_convert(pval++, iis2dh->acc[i], iis2dh->gain);
	}
}

static int iis2dh_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis2dh_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis2dh_config(const struct device *dev, enum sensor_channel chan,
			 enum sensor_attribute attr,
			 const struct sensor_value *val)
{
	switch (attr) {
#if (CONFIG_IIS2DH_RANGE == 0)
	case SENSOR_ATTR_FULL_SCALE:
		return iis2dh_set_range(dev, sensor_ms2_to_g(val));
#endif
#if (CONFIG_IIS2DH_ODR == 0)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis2dh_set_odr(dev, val->val1);
#endif
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis2dh_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return iis2dh_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int iis2dh_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct iis2dh_data *iis2dh = dev->data;
	int16_t buf[3];

	/* fetch raw data sample */
	if (iis2dh_acceleration_raw_get(iis2dh->ctx, buf) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	iis2dh->acc[0] = sys_le16_to_cpu(buf[0]);
	iis2dh->acc[1] = sys_le16_to_cpu(buf[1]);
	iis2dh->acc[2] = sys_le16_to_cpu(buf[2]);

	return 0;
}

static const struct sensor_driver_api iis2dh_driver_api = {
	.attr_set = iis2dh_attr_set,
#if CONFIG_IIS2DH_TRIGGER
	.trigger_set = iis2dh_trigger_set,
#endif /* CONFIG_IIS2DH_TRIGGER */
	.sample_fetch = iis2dh_sample_fetch,
	.channel_get = iis2dh_channel_get,
};

static int iis2dh_init_interface(const struct device *dev)
{
	int res;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	res = iis2dh_spi_init(dev);
	if (res) {
		return res;
	}
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	res = iis2dh_i2c_init(dev);
	if (res) {
		return res;
	}
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

	return 0;
}

static int iis2dh_init(const struct device *dev)
{
	struct iis2dh_data *iis2dh = dev->data;
	const struct iis2dh_device_config *cfg = dev->config;
	uint8_t wai;

	if (iis2dh_init_interface(dev)) {
		return -EINVAL;
	}

	/* check chip ID */
	if (iis2dh_device_id_get(iis2dh->ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != IIS2DH_ID) {
		LOG_ERR("Invalid chip ID: %02x", wai);
		return -EINVAL;
	}

	if (iis2dh_block_data_update_set(iis2dh->ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	if (iis2dh_operating_mode_set(iis2dh->ctx, cfg->pm)) {
		return -EIO;
	}

#if (CONFIG_IIS2DH_ODR != 0)
	/* set default odr and full scale for acc */
	if (iis2dh_data_rate_set(iis2dh->ctx, CONFIG_IIS2DH_ODR) < 0) {
		return -EIO;
	}
#endif

#if (CONFIG_IIS2DH_RANGE != 0)
	iis2dh_set_fs_raw(dev, CONFIG_IIS2DH_RANGE);
#endif

#ifdef CONFIG_IIS2DH_TRIGGER
	if (cfg->int_gpio.port) {
		if (iis2dh_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupts");
			return -EIO;
		}
	}
#endif /* CONFIG_IIS2DH_TRIGGER */

	return 0;
}

#define IIS2DH_SPI(inst)                                                                           \
	(.spi = SPI_DT_SPEC_INST_GET(                                                              \
		 0, SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),)

#define IIS2DH_I2C(inst) (.i2c = I2C_DT_SPEC_INST_GET(inst),)

#define IIS2DH_DEFINE(inst)									\
	static struct iis2dh_data iis2dh_data_##inst;						\
												\
	static const struct iis2dh_device_config iis2dh_device_config_##inst = {		\
		COND_CODE_1(DT_INST_ON_BUS(inst, i2c), IIS2DH_I2C(inst), ())			\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), IIS2DH_SPI(inst), ())			\
		.pm = CONFIG_IIS2DH_POWER_MODE,							\
		IF_ENABLED(CONFIG_IIS2DH_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, iis2dh_init, NULL,					\
			      &iis2dh_data_##inst, &iis2dh_device_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &iis2dh_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(IIS2DH_DEFINE)
