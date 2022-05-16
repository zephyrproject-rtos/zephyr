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

#include <zephyr/init.h>
#include <stdlib.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#elif DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif

#include "lis2dw12.h"

LOG_MODULE_REGISTER(LIS2DW12, CONFIG_SENSOR_LOG_LEVEL);

/**
 * lis2dw12_set_range - set full scale range for acc
 * @dev: Pointer to instance of struct device (I2C or SPI)
 * @range: Full scale range (2, 4, 8 and 16 G)
 */
static int lis2dw12_set_range(const struct device *dev, uint8_t fs)
{
	int err;
	struct lis2dw12_data *lis2dw12 = dev->data;
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t shift_gain = 0U;

	err = lis2dw12_full_scale_set(ctx, fs);

	if (cfg->pm == LIS2DW12_CONT_LOW_PWR_12bit) {
		shift_gain = LIS2DW12_SHFT_GAIN_NOLP1;
	}

	if (!err) {
		/* save internally gain for optimization */
		lis2dw12->gain =
			LIS2DW12_FS_TO_GAIN(fs, shift_gain);
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
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t val;

	/* check if power off */
	if (odr == 0U) {
		return lis2dw12_data_rate_set(ctx, LIS2DW12_XL_ODR_OFF);
	}

	val =  LIS2DW12_ODR_TO_REG(odr);
	if (val > LIS2DW12_XL_ODR_1k6Hz) {
		LOG_ERR("ODR too high");
		return -ENOTSUP;
	}

	return lis2dw12_data_rate_set(ctx, val);
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
		return lis2dw12_set_range(dev,
				LIS2DW12_FS_TO_REG(sensor_ms2_to_g(val)));
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lis2dw12_set_odr(dev, val->val1);
	default:
		LOG_DBG("Acc attribute not supported");
		break;
	}

	return -ENOTSUP;
}


static inline int32_t sensor_ms2_to_mg(const struct sensor_value *ms2)
{
	int64_t nano_ms2 = (ms2->val1 * 1000000LL + ms2->val2) * 1000LL;

	if (nano_ms2 > 0) {
		return (nano_ms2 + SENSOR_G / 2) / SENSOR_G;
	} else {
		return (nano_ms2 - SENSOR_G / 2) / SENSOR_G;
	}
}

#if CONFIG_LIS2DW12_THRESHOLD

/* Converts a lis2dw12_fs_t range to its value in milli-g
 * Range can be 2/4/8/16G
 */
#define FS_RANGE_TO_MG(fs_range)	((2U << fs_range) * 1000U)

/* Converts a range in mg to the lsb value for the WK_THS register
 * For the reg value: 1 LSB = 1/64 of FS
 * Range can be 2/4/8/16G
 */
#define MG_TO_WK_THS_LSB(range_mg)	(range_mg / 64)

/* Calculates the WK_THS reg value
 * from the threshold in mg and the lsb value in mg
 * with correct integer rounding
 */
#define THRESHOLD_MG_TO_WK_THS_REG(thr_mg, lsb_mg) \
	((thr_mg + (lsb_mg / 2)) / lsb_mg)

static int lis2dw12_attr_set_thresh(const struct device *dev,
					enum sensor_channel chan,
					enum sensor_attribute attr,
					const struct sensor_value *val)
{
	uint8_t reg;
	size_t ret;
	int lsb_mg;
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	LOG_DBG("%s on channel %d", __func__, chan);

	/* can only be set for all directions at once */
	if (chan != SENSOR_CHAN_ACCEL_XYZ) {
		return -EINVAL;
	}

	/* Configure wakeup threshold threshold. */
	lis2dw12_fs_t range;
	int err = lis2dw12_full_scale_get(ctx, &range);

	if (err) {
		return err;
	}

	uint32_t thr_mg = abs(sensor_ms2_to_mg(val));

	/* Check maximum value: depends on current FS value */
	if (thr_mg >= FS_RANGE_TO_MG(range)) {
		return -EINVAL;
	}

	/* The threshold is applied to both positive and negative data:
	 * for a wake-up interrupt generation at least one of the three axes must be
	 * bigger than the threshold.
	 */
	lsb_mg = MG_TO_WK_THS_LSB(FS_RANGE_TO_MG(range));
	reg = THRESHOLD_MG_TO_WK_THS_REG(thr_mg, lsb_mg);

	LOG_DBG("Threshold %d mg -> fs: %u mg -> reg = %d LSBs",
			thr_mg, FS_RANGE_TO_MG(range), reg);
	ret = 0;

	return lis2dw12_wkup_threshold_set(ctx, reg);
}
#endif

static int lis2dw12_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
#if CONFIG_LIS2DW12_THRESHOLD
	switch (attr) {
	case SENSOR_ATTR_UPPER_THRESH:
	case SENSOR_ATTR_LOWER_THRESH:
		return lis2dw12_attr_set_thresh(dev, chan, attr, val);
	default:
		/* Do nothing */
		break;
	}
#endif

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
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t shift;
	int16_t buf[3];

	/* fetch raw data sample */
	if (lis2dw12_acceleration_raw_get(ctx, buf) < 0) {
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

static int lis2dw12_set_power_mode(const struct device *dev,
				    lis2dw12_mode_t pm)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
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

	return lis2dw12_write_reg(ctx, LIS2DW12_CTRL1, &regval, 1);
}

static int lis2dw12_set_low_noise(const struct device *dev,
				  bool low_noise)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dw12_ctrl6_t ctrl6;
	int ret;

	ret = lis2dw12_read_reg(ctx, LIS2DW12_CTRL6, (uint8_t *)&ctrl6, 1);
	if (ret < 0) {
		return ret;
	}
	ctrl6.low_noise = low_noise;
	return lis2dw12_write_reg(ctx, LIS2DW12_CTRL6, (uint8_t *)&ctrl6, 1);
}

static int lis2dw12_init(const struct device *dev)
{
	const struct lis2dw12_device_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t wai;
	int ret;

	/* check chip ID */
	ret = lis2dw12_device_id_get(ctx, &wai);
	if (ret < 0) {
		LOG_ERR("Not able to read dev id");
		return ret;
	}

	if (wai != LIS2DW12_ID) {
		LOG_ERR("Invalid chip ID");
		return -EINVAL;
	}

	/* reset device */
	ret = lis2dw12_reset_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(100);

	ret = lis2dw12_block_data_update_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		LOG_ERR("Not able to set BDU");
		return ret;
	}

	/* set power mode */
	LOG_DBG("power-mode is %d", cfg->pm);
	ret = lis2dw12_set_power_mode(dev, cfg->pm);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("low noise is %d", cfg->low_noise);
	ret = lis2dw12_set_low_noise(dev, cfg->low_noise);
	if (ret < 0) {
		LOG_ERR("Failed to configure low_noise");
		return ret;
	}

	/* set default odr to 12.5Hz acc */
	ret = lis2dw12_set_odr(dev, 12);
	if (ret < 0) {
		LOG_ERR("odr init error (12.5 Hz)");
		return ret;
	}

	LOG_DBG("range is %d", cfg->range);
	ret = lis2dw12_set_range(dev, LIS2DW12_FS_TO_REG(cfg->range));
	if (ret < 0) {
		LOG_ERR("range init error %d", cfg->range);
		return ret;
	}

	LOG_DBG("bandwidth filter is %u", (int)cfg->bw_filt);
	lis2dw12_filter_bandwidth_set(ctx, cfg->bw_filt);

#ifdef CONFIG_LIS2DW12_TRIGGER
	ret = lis2dw12_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupts");
		return ret;
	}
#endif /* CONFIG_LIS2DW12_TRIGGER */

	LOG_DBG("high pass reference mode is %d", (int)cfg->hp_ref_mode);
	ret = lis2dw12_reference_mode_set(ctx, cfg->hp_ref_mode);
	if (ret < 0) {
		LOG_ERR("high pass reference mode config error %d", (int)cfg->hp_ref_mode);
		return ret;
	}

	LOG_DBG("high pass filter path is %d", (int)cfg->hp_filter_path);
	lis2dw12_fds_t fds = cfg->hp_filter_path ?
		LIS2DW12_HIGH_PASS_ON_OUT : LIS2DW12_LPF_ON_OUT;
	ret = lis2dw12_filter_path_set(ctx, fds);
	if (ret < 0) {
		LOG_ERR("filter path config error %d", (int)cfg->hp_filter_path);
		return ret;
	}

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LIS2DW12 driver enabled without any devices"
#endif

/*
 * Device creation macro, shared by LIS2DW12_DEFINE_SPI() and
 * LIS2DW12_DEFINE_I2C().
 */

#define LIS2DW12_DEVICE_INIT(inst)					\
	DEVICE_DT_INST_DEFINE(inst,					\
			    lis2dw12_init,				\
			    NULL,					\
			    &lis2dw12_data_##inst,			\
			    &lis2dw12_config_##inst,			\
			    POST_KERNEL,				\
			    CONFIG_SENSOR_INIT_PRIORITY,		\
			    &lis2dw12_driver_api);

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LIS2DW12_TAP
#define LIS2DW12_CONFIG_TAP(inst)					\
	.tap_mode = DT_INST_PROP(inst, tap_mode),			\
	.tap_threshold = DT_INST_PROP(inst, tap_threshold),		\
	.tap_shock = DT_INST_PROP(inst, tap_shock),			\
	.tap_latency = DT_INST_PROP(inst, tap_latency),			\
	.tap_quiet = DT_INST_PROP(inst, tap_quiet),
#else
#define LIS2DW12_CONFIG_TAP(inst)
#endif /* CONFIG_LIS2DW12_TAP */

#ifdef CONFIG_LIS2DW12_TRIGGER
#define LIS2DW12_CFG_IRQ(inst) \
	.gpio_int = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),		\
	.int_pin = DT_INST_PROP(inst, int_pin),
#else
#define LIS2DW12_CFG_IRQ(inst)
#endif /* CONFIG_LIS2DW12_TRIGGER */

#define LIS2DW12_SPI_OPERATION (SPI_WORD_SET(8) |			\
				SPI_OP_MODE_MASTER |			\
				SPI_MODE_CPOL |				\
				SPI_MODE_CPHA)				\

#define LIS2DW12_CONFIG_SPI(inst)					\
	{								\
		.ctx = {						\
			.read_reg =					\
			   (stmdev_read_ptr) stmemsc_spi_read,		\
			.write_reg =					\
			   (stmdev_write_ptr) stmemsc_spi_write,	\
			.handle =					\
			   (void *)&lis2dw12_config_##inst.stmemsc_cfg,	\
		},							\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   LIS2DW12_SPI_OPERATION,	\
					   0),				\
		},							\
		.pm = DT_INST_PROP(inst, power_mode),			\
		.range = DT_INST_PROP(inst, range),			\
		.bw_filt = DT_INST_PROP(inst, bw_filt),      \
		.low_noise = DT_INST_PROP(inst, low_noise),      \
		.hp_filter_path = DT_INST_PROP(inst, hp_filter_path),      \
		.hp_ref_mode = DT_INST_PROP(inst, hp_ref_mode), \
		.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),      \
		LIS2DW12_CONFIG_TAP(inst)				\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
			(LIS2DW12_CFG_IRQ(inst)), ())			\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LIS2DW12_CONFIG_I2C(inst)					\
	{								\
		.ctx = {						\
			.read_reg =					\
			   (stmdev_read_ptr) stmemsc_i2c_read,		\
			.write_reg =					\
			   (stmdev_write_ptr) stmemsc_i2c_write,	\
			.handle =					\
			   (void *)&lis2dw12_config_##inst.stmemsc_cfg,	\
		},							\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		.pm = DT_INST_PROP(inst, power_mode),			\
		.range = DT_INST_PROP(inst, range),			\
		.bw_filt = DT_INST_PROP(inst, bw_filt),      \
		.low_noise = DT_INST_PROP(inst, low_noise),      \
		.hp_filter_path = DT_INST_PROP(inst, hp_filter_path),      \
		.hp_ref_mode = DT_INST_PROP(inst, hp_ref_mode), \
		.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),      \
		LIS2DW12_CONFIG_TAP(inst)				\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, irq_gpios),	\
			(LIS2DW12_CFG_IRQ(inst)), ())			\
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LIS2DW12_DEFINE(inst)						\
	static struct lis2dw12_data lis2dw12_data_##inst;		\
	static const struct lis2dw12_device_config lis2dw12_config_##inst =	\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),				\
		    (LIS2DW12_CONFIG_SPI(inst)),			\
		    (LIS2DW12_CONFIG_I2C(inst)));			\
	LIS2DW12_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(LIS2DW12_DEFINE)
