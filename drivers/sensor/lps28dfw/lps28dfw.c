/* ST Microelectronics LPS28DFW pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps28dfw.pdf
 */

#define DT_DRV_COMPAT st_lps28dfw

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "lps28dfw.h"

#define LPS28DFW_SWRESET_WAIT_TIME	50

LOG_MODULE_REGISTER(LPS28DFW, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps28dfw_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps28dfw_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_md_t md;

	md.odr = odr;
	md.avg = cfg->avg;
	md.lpf = cfg->lpf;
	md.fs = cfg->fs;

	return lps28dfw_mode_set(ctx, &md);
}

static int lps28dfw_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps28dfw_data *data = dev->data;
	const struct lps28dfw_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_data_t raw_data;
	lps28dfw_md_t md;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (lps28dfw_data_get(ctx, &md, &raw_data) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_data.pressure.raw;
	data->sample_temp = raw_data.heat.raw;

	return 0;
}

static inline void lps28dfw_press_convert(const struct device *dev,
					 struct sensor_value *val,
					 int32_t raw_val)
{
	int32_t press_tmp = raw_val >> 8; /* raw value is left aligned (24 msb) */
	lps28dfw_md_t md;

	md.fs = ((struct lps28dfw_config *)dev->config)->fs;

	/* Pressure sensitivity is:
	 * - 4096 LSB/hPa for Full-Scale of 260 - 1260 hPa:
	 * - 2048 LSB/hPa for Full-Scale of 260 - 4060 hPa:
	 * Also convert hPa into kPa
	 */
	if (md.fs == 0) {
		val->val1 = press_tmp / 40960;
	} else if (md.fs == 1) {
		val->val1 = press_tmp / 20480;
	}

	/* For the decimal part use (3125 / 128) as a factor instead of
	 * (1000000 / 40960) to avoid int32 overflow
	 */
	val->val2 = (press_tmp % 40960) * 3125 / 128;
}

static inline void lps28dfw_temp_convert(struct sensor_value *val,
					int16_t raw_val)
{
	/* Temperature sensitivity is 100 LSB/deg C */
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int lps28dfw_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps28dfw_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		lps28dfw_press_convert(dev, val, data->sample_press);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		lps28dfw_temp_convert(val, data->sample_temp);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const uint16_t lps28dfw_map[] = {0, 1, 4, 10, 25, 50, 75, 100, 200};

static int lps28dfw_odr_set(const struct device *dev, uint16_t freq)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(lps28dfw_map); odr++) {
		if (freq == lps28dfw_map[odr]) {
			break;
		}
	}

	if (odr == ARRAY_SIZE(lps28dfw_map)) {
		LOG_DBG("bad frequency");
		return -EINVAL;
	}

	if (lps28dfw_set_odr_raw(dev, odr) < 0) {
		LOG_DBG("failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int lps28dfw_attr_set(const struct device *dev,
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
		return lps28dfw_odr_set(dev, val->val1);
	default:
		LOG_DBG("operation not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api lps28dfw_driver_api = {
	.attr_set = lps28dfw_attr_set,
	.sample_fetch = lps28dfw_sample_fetch,
	.channel_get = lps28dfw_channel_get,
#if CONFIG_LPS28DFW_TRIGGER
	.trigger_set = lps28dfw_trigger_set,
#endif
};

static int lps28dfw_init_chip(const struct device *dev)
{
	const struct lps28dfw_config * const cfg = dev->config;
	__maybe_unused struct lps22df_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_id_t id;
	lps28dfw_stat_t status;
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

	if (lps28dfw_id_get(ctx, &id) < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return -EIO;
	}

	if (id.whoami != LPS28DFW_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, id.whoami);
		return -EIO;
	}

	LOG_DBG("%s: chip id 0x%x", dev->name, id.whoami);

	/* Restore default configuration */
	if (lps28dfw_init_set(ctx, LPS28DFW_RESET) < 0) {
		LOG_ERR("%s: Not able to reset device", dev->name);
		return -EIO;
	}

	do {
		if (!--tries) {
			LOG_DBG("sw reset timed out");
			return -ETIMEDOUT;
		}
		k_usleep(LPS28DFW_SWRESET_WAIT_TIME);

		if (lps28dfw_status_get(ctx, &status) < 0) {
			return -EIO;
		}
	} while (status.sw_reset);

	/* Set bdu and if_inc recommended for driver usage */
	if (lps28dfw_init_set(ctx, LPS28DFW_DRV_RDY) < 0) {
		LOG_ERR("%s: Not able to set device to ready state", dev->name);
		return -EIO;
	}

	if (ON_I3C_BUS(cfg)) {
		lps28dfw_bus_mode_t bus_mode;

		/* Select bus interface */
		bus_mode.filter = LPS28DFW_AUTO;
		bus_mode.interface = LPS28DFW_SEL_BY_HW;
		lps28dfw_bus_mode_set(ctx, &bus_mode);

	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = lps28dfw_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to set odr %d", dev->name, cfg->odr);
		return ret;
	}

	return 0;
}

static int lps28dfw_init(const struct device *dev)
{
	if (lps28dfw_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_LPS28DFW_TRIGGER
	if (lps28dfw_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}

#ifdef CONFIG_LPS28DFW_TRIGGER
#define LPS28DFW_CFG_IRQ(inst) \
	.gpio_int = GPIO_DT_SPEC_INST_GET(inst, drdy_gpios),
#else
#define LPS28DFW_CFG_IRQ(inst)
#endif /* CONFIG_LPS28DFW_TRIGGER */

#define LPS28DFW_CONFIG_COMMON(inst)					\
	.odr = DT_INST_PROP(inst, odr),					\
	.lpf = DT_INST_PROP(inst, lpf),					\
	.avg = DT_INST_PROP(inst, avg),					\
	.fs = DT_INST_PROP(inst, fs),					\
	.drdy_pulsed = DT_INST_PROP(inst, drdy_pulsed),			\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, drdy_gpios),		\
		    (LPS28DFW_CFG_IRQ(inst)), ())

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LPS28DFW_CONFIG_I2C(inst)					\
	{								\
		STMEMSC_CTX_I2C(&lps28dfw_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i2c = I2C_DT_SPEC_INST_GET(inst),		\
		},							\
		LPS28DFW_CONFIG_COMMON(inst)				\
	}

/*
 * Instantiation macros used when a device is on an I#C bus.
 */

#define LPS28DFW_CONFIG_I3C(inst)					\
	{								\
		STMEMSC_CTX_I3C(&lps28dfw_config_##inst.stmemsc_cfg),	\
		.stmemsc_cfg = {					\
			.i3c = &lps28dfw_data_##inst.i3c_dev,		\
		},							\
		.i3c.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		\
		.i3c.dev_id = I3C_DEVICE_ID_DT_INST(inst),		\
		LPS28DFW_CONFIG_COMMON(inst)				\
	}

#define LPS28DFW_CONFIG_I3C_OR_I2C(inst)					\
	COND_CODE_0(DT_INST_PROP_BY_IDX(inst, reg, 1),			\
		    (LPS28DFW_CONFIG_I2C(inst)),				\
		    (LPS28DFW_CONFIG_I3C(inst)))

/*
 * Main instantiation macro. Use of COND_CODE_1() selects the right
 * bus-specific macro at preprocessor time.
 */

#define LPS28DFW_DEFINE(inst)							\
	static struct lps28dfw_data lps28dfw_data_##inst;				\
	static const struct lps28dfw_config lps28dfw_config_##inst =		\
	COND_CODE_1(DT_INST_ON_BUS(inst, i3c),			\
				 (LPS28DFW_CONFIG_I3C_OR_I2C(inst)),		\
				 (LPS28DFW_CONFIG_I2C(inst)));			\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps28dfw_init, NULL, &lps28dfw_data_##inst,	\
			      &lps28dfw_config_##inst, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY, &lps28dfw_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPS28DFW_DEFINE)
