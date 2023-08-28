/* ST Microelectronics LPS22DF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22df.pdf
 */

#define DT_DRV_COMPAT st_lps22df

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lps22df.h"

#define LPS22DF_SWRESET_WAIT_TIME	50

LOG_MODULE_REGISTER(LPS22DF, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps22df_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps22df_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_md_t md;

	md.odr = odr;
	md.avg = cfg->avg;
	md.lpf = cfg->lpf;

	return lps22df_mode_set(ctx, &md);
}

static int lps22df_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps22df_data *data = dev->data;
	const struct lps22df_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_data_t raw_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (lps22df_data_get(ctx, &raw_data) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_data.pressure.raw;
	data->sample_temp = raw_data.heat.raw;

	return 0;
}

static inline void lps22df_press_convert(struct sensor_value *val,
					 int32_t raw_val)
{
	int32_t press_tmp = raw_val >> 8; /* raw value is left aligned (24 msb) */

	/* Pressure sensitivity is 4096 LSB/hPa */
	/* Also convert hPa into kPa */

	val->val1 = press_tmp / 40960;

	/* For the decimal part use (3125 / 128) as a factor instead of
	 * (1000000 / 40960) to avoid int32 overflow
	 */
	val->val2 = (press_tmp % 40960) * 3125 / 128;
}

static inline void lps22df_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int lps22df_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22df_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps22df_press_convert(val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps22df_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const uint16_t lps22df_map[] = {0, 1, 4, 10, 25, 50, 75, 100, 200};

static int lps22df_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(lps22df_map); odr++) {
		if (freq == lps22df_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lps22df_map)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	if (lps22df_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int lps22df_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		LOG_WRN("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lps22df_odr_set(dev, val->val1);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps22df_driver_api = {
	.attr_set = lps22df_attr_set,
	.sample_fetch = lps22df_sample_fetch,
	.channel_get = lps22df_channel_get,
#if CONFIG_LPS22DF_TRIGGER
	.trigger_set = lps22df_trigger_set,
#endif
};

static int lps22df_init_chip(const struct device *dev)
{
	const struct lps22df_config * const cfg = dev->config;
	__maybe_unused struct lps22df_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps22df_id_t id;
	lps22df_stat_t status;
	uint8_t tries = 10;
	int ret;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i3c)
	if (cfg->i3c.bus != NULL) {
		/*
		 * Need to grab the pointer to the I3C device descriptor
		 * before we can talk to the sensor.
		 */
		data->i3c_dev = i3c_device_find(cfg->i3c.bus, &cfg->i3c.dev_id);
		if (data->i3c_dev == NULL) {
			LOG_ERR("Cannot find I3C device descriptor");
			return -ENODEV;
		}
	}
#endif

	if (lps22df_id_get(ctx, &id) < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return -EIO;
	}

	if (id.whoami != LPS22DF_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, id.whoami);
		return -EIO;
	}

	LOG_DBG("%s: chip id 0x%x", dev->name, id.whoami);

	/* Restore default configuration */
	if (lps22df_init_set(ctx, LPS22DF_RESET) < 0) {
		LOG_ERR("%s: Not able to reset device", dev->name);
		return -EIO;
	}

	do {
		if (!--tries) {
			LOG_DBG("sw reset timed out");
			return -ETIMEDOUT;
		}
		k_usleep(LPS22DF_SWRESET_WAIT_TIME);

		if (lps22df_status_get(ctx, &status) < 0) {
			return -EIO;
		}
	} while (status.sw_reset);

	/* Set bdu and if_inc recommended for driver usage */
	if (lps22df_init_set(ctx, LPS22DF_DRV_RDY) < 0) {
		LOG_ERR("%s: Not able to set device to ready state", dev->name);
		return -EIO;
	}

	if (ON_I3C_BUS(cfg)) {
		lps22df_bus_mode_t bus_mode;

		/* Select bus interface */
		bus_mode.filter = LPS22DF_AUTO;
		bus_mode.interface = LPS22DF_SEL_BY_HW;
		lps22df_bus_mode_set(ctx, &bus_mode);

	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = lps22df_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to set odr %d", dev->name, cfg->odr);
		return ret;
	}

	return 0;
}

static int lps22df_init(const struct device *dev)
{
	if (lps22df_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LPS22DF_TRIGGER
	if (lps22df_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

/*
 * Instantiation macros used when a device is on a SPI bus.
 */

#ifdef CONFIG_LPS22DF_TRIGGER
#define LPS22DF_CFG_IRQ(inst) \
	.gpio_int = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),
#else
#define LPS22DF_CFG_IRQ(inst)
#endif /* CONFIG_LPS22DF_TRIGGER */

#define LPS22DF_CONFIG_COMMON(inst)					\
	.odr = DT_INST_PROP(inst, odr),					\
	.lpf = DT_INST_PROP(inst, lpf),					\
	.avg = DT_INST_PROP(inst, avg),					\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),		\
		    (LPS22DF_CFG_IRQ(inst)), ())

#define LPS22DF_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER |	\
			       SPI_MODE_CPOL | SPI_MODE_CPHA)		\

#define LPS22DF_CONFIG_SPI(inst)					\
	{								\
		STMEMSC_CTX_SPI(&lps22df_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.spi = SPI_DT_SPEC_INST_GET(inst,		\
					   LPS22DF_SPI_OPERATION,	\
					   0),				\
		},							\
		LPS22DF_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LPS22DF_CONFIG_I2C(inst)					\
	{								\
		STMEMSC_CTX_I2C(&lps22df_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		LPS22DF_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I#C bus.
 */

#define LPS22DF_CONFIG_I3C(inst)					\
	{								\
		STMEMSC_CTX_I3C(&lps22df_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i3c = &lps22df_data_##inst.i3c_dev,		\
		},							\
		.i3c.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		\
		.i3c.dev_id = I3C_DEVICE_ID_DT_INST(inst),		\
		LPS22DF_CONFIG_COMMON(inst)				\
	}

#define LPS22DF_CONFIG_I3C_OR_I2C(inst)					\
	COND_CODE_0(DT_INST_PROP_BY_IDX(inst, reg, 1),			\
		    (LPS22DF_CONFIG_I2C(inst)),				\
		    (LPS22DF_CONFIG_I3C(inst)))

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LPS22DF_DEFINE(inst)							\
	static struct lps22df_data lps22df_data_##inst;				\
	static const struct lps22df_config lps22df_config_##inst =		\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
		    (LPS22DF_CONFIG_SPI(inst)),					\
		    (COND_CODE_1(DT_INST_ON_BUS(inst, i3c),			\
				 (LPS22DF_CONFIG_I3C_OR_I2C(inst)),		\
				 (LPS22DF_CONFIG_I2C(inst)))));			\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps22df_init, NULL, &lps22df_data_##inst,	\
			      &lps22df_config_##inst, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY, &lps22df_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPS22DF_DEFINE)
