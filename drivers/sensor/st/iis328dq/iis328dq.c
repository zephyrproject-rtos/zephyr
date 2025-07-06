/* ST Microelectronics IIS328DQ 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis328dq.pdf
 */

#define DT_DRV_COMPAT st_iis328dq

#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif

#include "iis328dq.h"

LOG_MODULE_REGISTER(IIS328DQ, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis328dq_set_odr - set new Full Scale (in ±g)
 */
static int iis328dq_set_range(const struct device *dev, uint8_t fs)
{
	int err;
	struct iis328dq_data *iis328dq = dev->data;
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	iis328dq_fs_t fs_reg;
	uint8_t gain;

	if (fs <= 2) {
		fs_reg = IIS328DQ_2g;
		gain = 1;
	} else if (fs <= 4) {
		fs_reg = IIS328DQ_4g;
		gain = 2;
	} else if (fs <= 8) {
		fs_reg = IIS328DQ_8g;
		gain = 4;
	} else {
		LOG_ERR("FS too high");
		return -ENOTSUP;
	}
	err = iis328dq_full_scale_set(ctx, fs_reg);

	if (!err) {
		iis328dq->gain = gain;
	}

	return err;
}

/**
 * iis328dq_set_odr - set new Output Data Rate/sampling frequency (in Hz)
 */
static int iis328dq_set_odr(const struct device *dev, uint16_t odr)
{
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	iis328dq_dr_t odr_reg;

	if (odr == 0U) {
		odr_reg = IIS328DQ_ODR_OFF;
	} else if (odr <= 1) {
		odr_reg = IIS328DQ_ODR_1Hz;
	} else if (odr <= 2) {
		odr_reg = IIS328DQ_ODR_2Hz;
	} else if (odr <= 5) {
		odr_reg = IIS328DQ_ODR_5Hz;
	} else if (odr <= 10) {
		odr_reg = IIS328DQ_ODR_10Hz;
	} else if (odr <= 50) {
		odr_reg = IIS328DQ_ODR_50Hz;
	} else if (odr <= 100) {
		odr_reg = IIS328DQ_ODR_100Hz;
	} else if (odr <= 400) {
		odr_reg = IIS328DQ_ODR_400Hz;
	} else if (odr <= 1000) {
		odr_reg = IIS328DQ_ODR_1kHz;
	} else {
		LOG_ERR("ODR too high");
		return -ENOTSUP;
	}

	if (iis328dq_data_rate_set(ctx, odr_reg) != 0) {
		LOG_ERR("Failed to set ODR");
		return -EIO;
	}

	return 0;
}

static inline void iis328dq_convert(struct sensor_value *val, int raw_val, uint8_t gain)
{
	int64_t dval;

	/* Gain is in mg/LSB */
	/* Convert to μm/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void iis328dq_channel_get_acc(const struct device *dev, enum sensor_channel chan,
					    struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis328dq_data *iis328dq = dev->data;
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
		ofs_start = 0U;
		ofs_stop = 2U;
		break;
	}

	for (i = ofs_start; i <= ofs_stop; i++) {
		iis328dq_convert(pval++, iis328dq->acc[i], iis328dq->gain);
	}
}

static int iis328dq_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis328dq_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

#ifdef CONFIG_IIS328DQ_THRESHOLD
static int iis328dq_set_threshold(const struct device *dev, bool is_lower,
				  const struct sensor_value *val)
{
	int err;
	const struct iis328dq_config *cfg = dev->config;
	struct iis328dq_data *iis328dq = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (val->val1 < 0 || val->val2 < 0) {
		/* thresholds are absolute */
		return -EINVAL;
	}
	int64_t micro_ms2 = (val->val1 * INT64_C(1000000)) + val->val2;
	/* factor guessed from similar-looking LIS2DH12 datasheet */
	uint8_t mg_per_digit = iis328dq->gain * 16;

	int16_t val_raw = (micro_ms2 * 1000LL) / SENSOR_G / mg_per_digit;

	if (is_lower) {
		/* internal INT1 handles lower threshold */
		err = iis328dq_int1_treshold_set(ctx, val_raw);
		if (err) {
			LOG_ERR("Could not set INT1_THS to 0x%02X, error %d", val_raw, err);
			return err;
		}
	} else {
		/* internal INT2 handles lower threshold */
		err = iis328dq_int2_treshold_set(ctx, val_raw);
		if (err) {
			LOG_ERR("Could not set INT2_THS to 0x%02X, error %d", val_raw, err);
			return err;
		}
	}
	return 0;
}

static int iis328dq_set_duration(const struct device *dev, uint16_t dur)
{
	int err;
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	if (dur > 0x7F) {
		LOG_WRN("Duration value %u too large", dur);
		return -EINVAL;
	}

	err = iis328dq_int1_dur_set(ctx, dur);
	if (err) {
		LOG_ERR("Could not set INT1_DUR to 0x%02X, error %d", dur, err);
		return err;
	}
	err = iis328dq_int2_dur_set(ctx, dur);
	if (err) {
		LOG_ERR("Could not set INT2_DUR to 0x%02X, error %d", dur, err);
		return err;
	}
	return 0;
}
#endif /* CONFIG_IIS328DQ_THRESHOLD */
#define IIS328DQ_ATTR_DURATION SENSOR_ATTR_PRIV_START

static int iis328dq_dev_config(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return iis328dq_set_range(dev, sensor_ms2_to_g(val));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis328dq_set_odr(dev, val->val1);
#ifdef CONFIG_IIS328DQ_THRESHOLD
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		if (chan != SENSOR_CHAN_ACCEL_XYZ) {
			LOG_ERR("Threshold cannot be set per-channel");
			return -ENOTSUP;
		}
		return iis328dq_set_threshold(dev, attr == SENSOR_ATTR_LOWER_THRESH, val);
	case IIS328DQ_ATTR_DURATION:
		if (chan != SENSOR_CHAN_ACCEL_XYZ) {
			LOG_ERR("Duration cannot be set per-channel");
			return -ENOTSUP;
		}
		return iis328dq_set_duration(dev, val->val1);
#endif /* CONFIG_IIS328DQ_THRESHOLD */
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis328dq_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
	case SENSOR_CHAN_ALL:
		return iis328dq_dev_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int iis328dq_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct iis328dq_data *iis328dq = dev->data;
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int16_t buf[3];

	/* fetch raw data sample */
	if (iis328dq_acceleration_raw_get(ctx, buf) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	iis328dq->acc[0] = buf[0] >> 4;
	iis328dq->acc[1] = buf[1] >> 4;
	iis328dq->acc[2] = buf[2] >> 4;

	return 0;
}

static DEVICE_API(sensor, iis328dq_driver_api) = {
	.attr_set = iis328dq_attr_set,
#if CONFIG_IIS328DQ_TRIGGER
	.trigger_set = iis328dq_trigger_set,
#endif /* CONFIG_IIS328DQ_TRIGGER */
	.sample_fetch = iis328dq_sample_fetch,
	.channel_get = iis328dq_channel_get,
};

static int iis328dq_init(const struct device *dev)
{
	struct iis328dq_data *iis328dq = dev->data;
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t reg_value;

	iis328dq->dev = dev;

	/* check chip ID */
	if (iis328dq_device_id_get(ctx, &reg_value) < 0) {
		return -EIO;
	}

	if (reg_value != IIS328DQ_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	if (iis328dq_boot_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	k_sleep(K_MSEC(100));

	if (iis328dq_boot_get(ctx, &reg_value) < 0) {
		return -EIO;
	}
	if (reg_value != PROPERTY_DISABLE) {
		LOG_ERR("BOOT did not deassert");
		return -EIO;
	}

	if (iis328dq_block_data_update_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	/* set default odr to 12.5Hz acc */
	if (iis328dq_set_odr(dev, 12) < 0) {
		LOG_ERR("odr init error (12.5 Hz)");
		return -EIO;
	}

	if (iis328dq_set_range(dev, cfg->range) < 0) {
		LOG_ERR("range init error %d", cfg->range);
		return -EIO;
	}

#ifdef CONFIG_IIS328DQ_TRIGGER
	if (iis328dq_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}
#endif /* CONFIG_IIS328DQ_TRIGGER */

	return 0;
}

/*
 * Device creation macro, shared by IIS328DQ_DEFINE_SPI() and
 * IIS328DQ_DEFINE_I2C().
 */

#define IIS328DQ_DEVICE_INIT(inst)                                                                 \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, iis328dq_init, NULL, &iis328dq_data_##inst,             \
				     &iis328dq_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &iis328dq_driver_api);

#ifdef CONFIG_IIS328DQ_TRIGGER
#ifdef CONFIG_IIS328DQ_THRESHOLD
#define IIS328DQ_CFG_IRQ_THRESHOLD(inst)                                                           \
	.threshold_pad = DT_INST_PROP_OR(inst, threshold_int_pad, -1),
#else
#define IIS328DQ_CFG_IRQ_THRESHOLD(inst)
#endif /* CONFIG_IIS328DQ_THRESHOLD */

#define IIS328DQ_CFG_IRQ(inst)                                                                     \
	.gpio_int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),                              \
	.gpio_int2 = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),                              \
	.drdy_pad = DT_INST_PROP_OR(inst, drdy_int_pad, -1), IIS328DQ_CFG_IRQ_THRESHOLD(inst)
#else
#define IIS328DQ_CFG_IRQ(inst)
#endif /* CONFIG_IIS328DQ_TRIGGER */

#define IIS328DQ_CONFIG_COMMON(inst)                                                               \
	.range = DT_INST_PROP(inst, range),                                                        \
	IF_ENABLED(UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int1_gpios),                                \
			   DT_INST_NODE_HAS_PROP(inst, int2_gpios)),                               \
		   (IIS328DQ_CFG_IRQ(inst)))

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define IIS328DQ_SPI_OPERATION                                                                     \
	(SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define IIS328DQ_CONFIG_SPI(inst)                                                                  \
	{                                                                                          \
		STMEMSC_CTX_SPI_INCR(&iis328dq_config_##inst.stmemsc_cfg),                         \
			.stmemsc_cfg =                                                             \
				{                                                                  \
					.spi = SPI_DT_SPEC_INST_GET(inst, IIS328DQ_SPI_OPERATION,  \
								    0),                            \
				},                                                                 \
			IIS328DQ_CONFIG_COMMON(inst)                                               \
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define IIS328DQ_CONFIG_I2C(inst)                                                                  \
	{                                                                                          \
		STMEMSC_CTX_I2C_INCR(&iis328dq_config_##inst.stmemsc_cfg),                         \
			.stmemsc_cfg =                                                             \
				{                                                                  \
					.i2c = I2C_DT_SPEC_INST_GET(inst),                         \
				},                                                                 \
			IIS328DQ_CONFIG_COMMON(inst)                                               \
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define IIS328DQ_DEFINE(inst)                                                                      \
	static struct iis328dq_data iis328dq_data_##inst;                                          \
	static const struct iis328dq_config iis328dq_config_##inst =                               \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (IIS328DQ_CONFIG_SPI(inst)),                \
			    (IIS328DQ_CONFIG_I2C(inst)));                                          \
	IIS328DQ_DEVICE_INIT(inst)                                                                 \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, drdy_int_pad),                                      \
		   (BUILD_ASSERT(                                                                  \
			    DT_INST_NODE_HAS_PROP(                                                 \
				    inst, CONCAT(int, DT_INST_PROP(inst, drdy_int_pad), _gpios)),  \
			    "No GPIO pin defined for IIS328DQ DRDY interrupt");))                  \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, threshold_int_pad),                                 \
		   (BUILD_ASSERT(DT_INST_NODE_HAS_PROP(                                            \
					 inst, CONCAT(int, DT_INST_PROP(inst, threshold_int_pad),  \
						      _gpios)),                                    \
				 "No GPIO pin defined for IIS328DQ threshold interrupt");))        \
	IF_ENABLED(                                                                                \
		UTIL_AND(DT_INST_NODE_HAS_PROP(inst, drdy_int_pad),                                \
			 DT_INST_NODE_HAS_PROP(inst, threshold_int_pad)),                          \
		(BUILD_ASSERT(                                                                     \
			 DT_INST_PROP(inst, drdy_int_pad) !=                                       \
				 DT_INST_PROP(inst, threshold_int_pad),                            \
			 "IIS328DQ DRDY interrupt and threshold interrupt cannot share a pin");))

DT_INST_FOREACH_STATUS_OKAY(IIS328DQ_DEFINE)
