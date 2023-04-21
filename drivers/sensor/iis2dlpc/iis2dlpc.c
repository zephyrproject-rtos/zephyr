/* ST Microelectronics IIS2DLPC 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dlpc.pdf
 */

#define DT_DRV_COMPAT st_iis2dlpc

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

#include "iis2dlpc.h"

LOG_MODULE_REGISTER(IIS2DLPC, CONFIG_SENSOR_LOG_LEVEL);

/**
 * iis2dlpc_set_range - set full scale range for acc
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int iis2dlpc_set_range(const struct device *dev, uint8_t fs)
{
	int err;
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t shift_gain = 0U;

	err = iis2dlpc_full_scale_set(ctx, fs);

	if (cfg->pm == IIS2DLPC_CONT_LOW_PWR_12bit) {
		shift_gain = IIS2DLPC_SHFT_GAIN_NOLP1;
	}

	if (!err) {
		/* save internally gain for optimization */
		iis2dlpc->gain = IIS2DLPC_FS_TO_GAIN(fs, shift_gain);
	}

	return err;
}

/**
 * iis2dlpc_set_odr - set new sampling frequency
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @odr: Output data rate
 */
static int iis2dlpc_set_odr(const struct device *dev, uint16_t odr)
{
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t val;

	/* check if power off */
	if (odr == 0U) {
		return iis2dlpc_data_rate_set(ctx, IIS2DLPC_XL_ODR_OFF);
	}

	val = IIS2DLPC_ODR_TO_REG(odr);
	if (val > IIS2DLPC_XL_ODR_1k6Hz) {
		LOG_ERR("ODR too high");
		return -ENOTSUP;
	}

	return iis2dlpc_data_rate_set(ctx, val);
}

static inline void iis2dlpc_convert(struct sensor_value *val, int raw_val,
				    float gain)
{
	int64_t dval;

	/* Gain is in ug/LSB */
	/* Convert to m/s^2 */
	dval = ((int64_t)raw_val * gain * SENSOR_G) / 1000000LL;
	val->val1 = dval / 1000000LL;
	val->val2 = dval % 1000000LL;
}

static inline void iis2dlpc_channel_get_acc(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	int i;
	uint8_t ofs_start, ofs_stop;
	struct iis2dlpc_data *iis2dlpc = dev->data;
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
		iis2dlpc_convert(pval++, iis2dlpc->acc[i], iis2dlpc->gain);
	}
}

static int iis2dlpc_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		iis2dlpc_channel_get_acc(dev, chan, val);
		return 0;
	default:
		LOG_DBG("Channel not supported");
		break;
	}

	return -ENOTSUP;
}

#ifdef CONFIG_IIS2DLPC_ACTIVITY
static int ii2sdlpc_set_slope_th(const struct device *dev, uint16_t th)
{
	int err;
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *) &cfg->ctx;

	err = iis2dlpc_wkup_threshold_set(ctx, th & 0x3F);
	if (err) {
		LOG_ERR("Could not set WK_THS to 0x%02X, error %d",
			th & 0x03, err);
		return err;
	}
	return 0;
}
static int ii2sdlpc_set_slope_dur(const struct device *dev, uint16_t dur)
{
	int err;
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *) &cfg->ctx;
	uint8_t val;

	val = (dur & 0x0F);
	err = iis2dlpc_act_sleep_dur_set(ctx, val);
	if (err) {
		LOG_ERR("Could not set SLEEP_DUR to 0x%02X, error %d",
			val, err);
		return err;
	}
	val = ((dur >> 5) & 0x03);
	err = iis2dlpc_wkup_dur_set(ctx, val);
	if (err) {
		LOG_ERR("Could not set WAKE_DUR to 0x%02X, error %d",
			val, err);
		return err;
	}
	return 0;
}
#endif /* CONFIG_IIS2DLPC_ACTIVITY */

static int iis2dlpc_dev_config(const struct device *dev,
			       enum sensor_channel chan,
			       enum sensor_attribute attr,
			       const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return iis2dlpc_set_range(dev,
				IIS2DLPC_FS_TO_REG(sensor_ms2_to_g(val)));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return iis2dlpc_set_odr(dev, val->val1);
#ifdef CONFIG_IIS2DLPC_ACTIVITY
	case SENSOR_ATTR_SLOPE_TH:
		return ii2sdlpc_set_slope_th(dev, val->val1);
	case SENSOR_ATTR_SLOPE_DUR:
		return ii2sdlpc_set_slope_dur(dev, val->val1);
#endif /* CONFIG_IIS2DLPC_ACTIVITY */
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}

static int iis2dlpc_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return iis2dlpc_dev_config(dev, chan, attr, val);
	default:
		LOG_DBG("Attr not supported on %d channel", chan);
		break;
	}

	return -ENOTSUP;
}

static int iis2dlpc_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t shift;
	int16_t buf[3];

	/* fetch raw data sample */
	if (iis2dlpc_acceleration_raw_get(ctx, buf) < 0) {
		LOG_DBG("Failed to fetch raw data sample");
		return -EIO;
	}

	/* adjust to resolution */
	if (cfg->pm == IIS2DLPC_CONT_LOW_PWR_12bit) {
		shift = IIS2DLPC_SHIFT_PM1;
	} else {
		shift = IIS2DLPC_SHIFT_PMOTHER;
	}

	iis2dlpc->acc[0] = sys_le16_to_cpu(buf[0]) >> shift;
	iis2dlpc->acc[1] = sys_le16_to_cpu(buf[1]) >> shift;
	iis2dlpc->acc[2] = sys_le16_to_cpu(buf[2]) >> shift;

	return 0;
}

static const struct sensor_driver_api iis2dlpc_driver_api = {
	.attr_set = iis2dlpc_attr_set,
#if CONFIG_IIS2DLPC_TRIGGER
	.trigger_set = iis2dlpc_trigger_set,
#endif /* CONFIG_IIS2DLPC_TRIGGER */
	.sample_fetch = iis2dlpc_sample_fetch,
	.channel_get = iis2dlpc_channel_get,
};

static int iis2dlpc_set_power_mode(stmdev_ctx_t *ctx, iis2dlpc_mode_t pm)
{
	uint8_t regval = IIS2DLPC_CONT_LOW_PWR_12bit;

	switch (pm) {
	case IIS2DLPC_CONT_LOW_PWR_2:
	case IIS2DLPC_CONT_LOW_PWR_3:
	case IIS2DLPC_CONT_LOW_PWR_4:
	case IIS2DLPC_HIGH_PERFORMANCE:
		regval = pm;
		break;
	default:
		LOG_DBG("Apply default Power Mode");
		break;
	}

	return iis2dlpc_write_reg(ctx, IIS2DLPC_CTRL1, &regval, 1);
}

static int iis2dlpc_init(const struct device *dev)
{
	struct iis2dlpc_data *iis2dlpc = dev->data;
	const struct iis2dlpc_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t wai;

	iis2dlpc->dev = dev;

	/* check chip ID */
	if (iis2dlpc_device_id_get(ctx, &wai) < 0) {
		return -EIO;
	}

	if (wai != IIS2DLPC_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	if (iis2dlpc_reset_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	k_busy_wait(100);

	if (iis2dlpc_block_data_update_set(ctx, PROPERTY_ENABLE) < 0) {
		return -EIO;
	}

	/* set power mode */
	LOG_INF("power-mode is %d", cfg->pm);
	if (iis2dlpc_set_power_mode(ctx, cfg->pm)) {
		return -EIO;
	}

	/* set default odr to 12.5Hz acc */
	if (iis2dlpc_set_odr(dev, 12) < 0) {
		LOG_ERR("odr init error (12.5 Hz)");
		return -EIO;
	}

	LOG_INF("range is %d", cfg->range);
	if (iis2dlpc_set_range(dev, IIS2DLPC_FS_TO_REG(cfg->range)) < 0) {
		LOG_ERR("range init error %d", cfg->range);
		return -EIO;
	}

#ifdef CONFIG_IIS2DLPC_TRIGGER
	if (iis2dlpc_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return -EIO;
	}

#ifdef CONFIG_IIS2DLPC_TAP
	LOG_INF("TAP: tap mode is %d", cfg->tap_mode);
	if (iis2dlpc_tap_mode_set(ctx, cfg->tap_mode) < 0) {
		LOG_ERR("Failed to select tap trigger mode");
		return -EIO;
	}

	LOG_INF("TAP: ths_x is %02x", cfg->tap_threshold[0]);
	if (iis2dlpc_tap_threshold_x_set(ctx, cfg->tap_threshold[0]) < 0) {
		LOG_ERR("Failed to set tap X axis threshold");
		return -EIO;
	}

	LOG_INF("TAP: ths_y is %02x", cfg->tap_threshold[1]);
	if (iis2dlpc_tap_threshold_y_set(ctx, cfg->tap_threshold[1]) < 0) {
		LOG_ERR("Failed to set tap Y axis threshold");
		return -EIO;
	}

	LOG_INF("TAP: ths_z is %02x", cfg->tap_threshold[2]);
	if (iis2dlpc_tap_threshold_z_set(ctx, cfg->tap_threshold[2]) < 0) {
		LOG_ERR("Failed to set tap Z axis threshold");
		return -EIO;
	}

	if (cfg->tap_threshold[0] > 0) {
		LOG_INF("TAP: tap_x enabled");
		if (iis2dlpc_tap_detection_on_x_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on X axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[1] > 0) {
		LOG_INF("TAP: tap_y enabled");
		if (iis2dlpc_tap_detection_on_y_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on Y axis");
			return -EIO;
		}
	}

	if (cfg->tap_threshold[2] > 0) {
		LOG_INF("TAP: tap_z enabled");
		if (iis2dlpc_tap_detection_on_z_set(ctx, 1) < 0) {
			LOG_ERR("Failed to set tap detection on Z axis");
			return -EIO;
		}
	}

	LOG_INF("TAP: shock is %02x", cfg->tap_shock);
	if (iis2dlpc_tap_shock_set(ctx, cfg->tap_shock) < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return -EIO;
	}

	LOG_INF("TAP: latency is %02x", cfg->tap_latency);
	if (iis2dlpc_tap_dur_set(ctx, cfg->tap_latency) < 0) {
		LOG_ERR("Failed to set tap latency");
		return -EIO;
	}

	LOG_INF("TAP: quiet time is %02x", cfg->tap_quiet);
	if (iis2dlpc_tap_quiet_set(ctx, cfg->tap_quiet) < 0) {
		LOG_ERR("Failed to set tap quiet time");
		return -EIO;
	}
#endif /* CONFIG_IIS2DLPC_TAP */
#endif /* CONFIG_IIS2DLPC_TRIGGER */

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "IIS2DLPC driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by IIS2DLPC_DEFINE_SPI() and
 * IIS2DLPC_DEFINE_I2C().
 */

#define IIS2DLPC_DEVICE_INIT(inst)					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			    iis2dlpc_init,				\
			    NULL,					\
			    &iis2dlpc_data_##inst,			\
			    &iis2dlpc_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &iis2dlpc_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_IIS2DLPC_TAP
#define IIS2DLPC_CONFIG_TAP(inst)					\
	.tap_mode = DT_INST_PROP(inst, tap_mode),			\
	.tap_threshold = DT_INST_PROP(inst, tap_threshold),		\
	.tap_shock = DT_INST_PROP(inst, tap_shock),			\
	.tap_latency = DT_INST_PROP(inst, tap_latency),			\
	.tap_quiet = DT_INST_PROP(inst, tap_quiet),
#else
#define IIS2DLPC_CONFIG_TAP(inst)
#endif /* CONFIG_IIS2DLPC_TAP */

#ifdef CONFIG_IIS2DLPC_TRIGGER
#define IIS2DLPC_CFG_IRQ(inst) \
	.gpio_drdy = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),		\
	.drdy_int = DT_INST_PROP(inst, drdy_int),
#else
#define IIS2DLPC_CFG_IRQ(inst)
#endif /* CONFIG_IIS2DLPC_TRIGGER */

#define IIS2DLPC_CONFIG_COMMON(inst)					\
	.pm = DT_INST_PROP(inst, power_mode),				\
	.range = DT_INST_PROP(inst, range),				\
	IIS2DLPC_CONFIG_TAP(inst)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),		\
		(IIS2DLPC_CFG_IRQ(inst)), ())

#define IIS2DLPC_SPI_OPERATION (SPI_WORD_SET(8) |			\
				SPI_OP_MODE_MASTER |			\
				SPI_MODE_CPOL |				\
				SPI_MODE_CPHA)				\

#define IIS2DLPC_CONFIG_SPI(inst)					\
	{								\
		STMEMSC_CTX_SPI(&iis2dlpc_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   IIS2DLPC_SPI_OPERATION,	\
					   0),				\
		},							\
		IIS2DLPC_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define IIS2DLPC_CONFIG_I2C(inst)					\
	{								\
		STMEMSC_CTX_I2C(&iis2dlpc_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		IIS2DLPC_CONFIG_COMMON(inst)				\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define IIS2DLPC_DEFINE(inst)						\
	static struct iis2dlpc_data iis2dlpc_data_##inst;		\
	static const struct iis2dlpc_config iis2dlpc_config_##inst =	\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (IIS2DLPC_CONFIG_SPI(inst)),			\
		    (IIS2DLPC_CONFIG_I2C(inst)));			\
	IIS2DLPC_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(IIS2DLPC_DEFINE)
