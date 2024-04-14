/* lps22hb.c - Driver for LPS22HB pressure and temperature sensor */

/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lps22hb_press

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lps22hb.h"

LOG_MODULE_REGISTER(LPS22HB, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps22hb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps22hb_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	return lps22hb_data_rate_set(ctx, odr);
}

static int lps22hb_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lps22hb_data *data = dev->data;
	const struct lps22hb_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint32_t raw_press;
	int16_t raw_temp;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (lps22hb_pressure_raw_get(ctx, &raw_press) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	if (lps22hb_temperature_raw_get(ctx, &raw_temp) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_press;
	data->sample_temp = raw_temp;

	return 0;
}

static inline void lps22hb_press_convert(struct sensor_value *val, int32_t raw_val)
{
	/* Pressure sensitivity is 4096 LSB/hPa */
	/* raw value is shifted one byte by the stmemc driver*/
	/* Convert raw_val to val in kPa */
	raw_val = raw_val >> 8;
	val->val1 = (raw_val >> 12) / 10;
	val->val2 =
		(raw_val >> 12) % 10 * 100000 + (((int32_t)((raw_val) & 0x0FFF) * 100000L) >> 12);
}

static inline void lps22hb_temp_convert(struct sensor_value *val, int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int lps22hb_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hb_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22hb_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps22hb_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const uint16_t lps22hb_map[] = {0, 1, 10, 25, 50, 75};

static int lps22hb_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(lps22hb_map); odr++) {
		if (freq == lps22hb_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lps22hb_map)) {
		LOG_ERR("bad frequency");
		return -EINVAL;
	}

	if (lps22hb_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int lps22hb_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lps22hb_odr_set(dev, val->val1);
	default:
		LOG_ERR("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps22hb_driver_api = {
	.attr_set = lps22hb_attr_set,
	.sample_fetch = lps22hb_sample_fetch,
	.channel_get = lps22hb_channel_get,
#if CONFIG_LPS22HB_TRIGGER
	.trigger_set = lps22hb_trigger_set,
#endif
};

static int lps22hb_init_chip(const struct device *dev)
{
	const struct lps22hb_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id;
	int ret;

	if (lps22hb_device_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return -EIO;
	}

	if (chip_id != LPS22HB_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, chip_id);
		return -EIO;
	}

	LOG_DBG("%s: chip id 0x%x", dev->name, chip_id);

	if (lps22hb_reset_set(ctx, 1) < 0) {
		LOG_ERR("%s: Not able to reset device", dev->name);
		return -EIO;
	}

	/* configure sensor low-pass filter */
	if (cfg->low_pass_enabled) {
		ret = lps22hb_low_pass_filter_mode_set(ctx, cfg->filter_mode);
		if (ret < 0) {
			LOG_ERR("%s: Failed to set low-pass filter (mode=%d)", dev->name,
				cfg->filter_mode);
			return ret;
		}
		LOG_INF("%s: Low-pass filter enabled (mode=%d)", dev->name,
			cfg->filter_mode);
	} else {
		LOG_INF("%s: Low-pass filter disabled", dev->name);
	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = lps22hb_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to set odr %d", dev->name, cfg->odr);
		return ret;
	}

	if (lps22hb_block_data_update_set(ctx, PROPERTY_ENABLE) < 0) {
		LOG_ERR("%s: Failed to set BDU", dev->name);
		return ret;
	}

	return 0;
}

static int lps22hb_init(const struct device *dev)
{
	if (lps22hb_init_chip(dev) < 0) {
		LOG_ERR("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LPS22HB_TRIGGER
	if (lps22hb_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LPS22HB driver enabled without any devices"
#endif

#ifdef CONFIG_LPS22HB_TRIGGER
#define LPS22HB_CFG_IRQ(inst) .gpio_int = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),
#else
#define LPS22HB_CFG_IRQ(inst)
#endif /* CONFIG_LPS22HB_TRIGGER */

#define LPS22HB_CONFIG_COMMON(inst)                                                                \
	.odr = DT_INST_PROP(inst, odr), .low_pass_enabled = DT_INST_PROP(inst, lpf_en),            \
	.filter_mode = DT_INST_PROP(inst, lpf_mode),                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios), (LPS22HB_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#define LPS22HB_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA)

#define LPS22HB_CONFIG_SPI(inst)                                                                   \
	{                                                                                          \
		STMEMSC_CTX_SPI(&lps22hb_config_##inst.stmemsc_cfg),                               \
			.stmemsc_cfg =                                                             \
				{                                                                  \
					.spi = SPI_DT_SPEC_INST_GET(inst, LPS22HB_SPI_OPERATION,   \
								    0),                            \
				},                                                                 \
			LPS22HB_CONFIG_COMMON(inst)                                                \
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LPS22HB_CONFIG_I2C(inst)                                                                   \
	{                                                                                          \
		STMEMSC_CTX_I2C(&lps22hb_config_##inst.stmemsc_cfg),                               \
			.stmemsc_cfg =                                                             \
				{                                                                  \
					.i2c = I2C_DT_SPEC_INST_GET(inst),                         \
				},                                                                 \
			LPS22HB_CONFIG_COMMON(inst)                                                \
	}

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LPS22HB_DEFINE(inst)                                                                       \
	static struct lps22hb_data lps22hb_data_##inst;                                            \
	static const struct lps22hb_config lps22hb_config_##inst =                                 \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi), (LPS22HB_CONFIG_SPI(inst)),                 \
			    (LPS22HB_CONFIG_I2C(inst)));                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps22hb_init, NULL, &lps22hb_data_##inst,               \
				     &lps22hb_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &lps22hb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPS22HB_DEFINE)
