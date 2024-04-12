/* ST Microelectronics LPS28DFW pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lps2xdf.h"
#include "lps28dfw.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LPS2XDF, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps28dfw_mode_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_md_t md;

	md.odr = odr;
	md.avg = cfg->avg;
	md.lpf = cfg->lpf;
	md.fs = cfg->fs;

	return lps28dfw_mode_set(ctx, &md);
}

static int lps28dfw_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lps2xdf_data *data = dev->data;
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_data_t raw_data;
	lps28dfw_md_t md;

	md.fs = cfg->fs;

	if (lps28dfw_data_get(ctx, &md, &raw_data) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_data.pressure.raw;
	data->sample_temp = raw_data.heat.raw;

	return 0;
}

/**
 * lps28dfw_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
#ifdef CONFIG_LPS2XDF_TRIGGER
static void lps28dfw_handle_interrupt(const struct device *dev)
{
	int ret;
	struct lps2xdf_data *lps28dfw = dev->data;
	const struct lps2xdf_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_all_sources_t status;

	if (lps28dfw_all_sources_get(ctx, &status) < 0) {
		LOG_DBG("failed reading status reg");
		goto exit;
	}

	if (status.drdy_pres == 0) {
		goto exit; /* spurious interrupt */
	}

	if (lps28dfw->handler_drdy != NULL) {
		lps28dfw->handler_drdy(dev, lps28dfw->data_ready_trigger);
	}

	if (ON_I3C_BUS(cfg)) {
		/*
		 * I3C IBI does not rely on GPIO.
		 * So no need to enable GPIO pin for interrupt trigger.
		 */
		return;
	}

exit:
	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

/**
 * lps28dfw_enable_int - enable selected int pin to generate interrupt
 */
static int lps28dfw_enable_int(const struct device *dev, int enable)
{
	const struct lps2xdf_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_pin_int_route_t int_route;

	/* set interrupt */
	lps28dfw_pin_int_route_get(ctx, &int_route);
	int_route.drdy_pres = enable;
	return lps28dfw_pin_int_route_set(ctx, &int_route);
}

/**
 * lps22df_trigger_set - link external trigger to event data ready
 */
static int lps28dfw_trigger_set(const struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps2xdf_data *lps28dfw = dev->data;
	const struct lps2xdf_config * const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_data_t raw_data;
	lps28dfw_md_t md;

	md.fs = cfg->fs;

	if (trig->chan != SENSOR_CHAN_ALL) {
		LOG_WRN("trigger set not supported on this channel.");
		return -ENOTSUP;
	}

	lps28dfw->handler_drdy = handler;
	lps28dfw->data_ready_trigger = trig;
	if (handler) {
		/* dummy read: re-trigger interrupt */
		if (lps28dfw_data_get(ctx, &md, &raw_data) < 0) {
			LOG_DBG("Failed to read sample");
			return -EIO;
		}
		return lps28dfw_enable_int(dev, 1);
	} else {
		return lps28dfw_enable_int(dev, 0);
	}

	return -ENOTSUP;
}
#endif /* CONFIG_LPS2XDF_TRIGGER */

const struct lps2xdf_chip_api st_lps28dfw_chip_api = {
	.mode_set_odr_raw = lps28dfw_mode_set_odr_raw,
	.sample_fetch = lps28dfw_sample_fetch,
#if CONFIG_LPS2XDF_TRIGGER
	.handle_interrupt = lps28dfw_handle_interrupt,
	.trigger_set = lps28dfw_trigger_set,
#endif
};

int st_lps28dfw_init(const struct device *dev)
{
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lps28dfw_id_t id;
	lps28dfw_stat_t status;
	uint8_t tries = 10;
	int ret;

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_lps28dfw, i3c)
	if (cfg->i3c.bus != NULL) {
		struct lps2xdf_data *data = dev->data;
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
		k_usleep(LPS2XDF_SWRESET_WAIT_TIME_US);

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
		lps28dfw_bus_mode_get(ctx, &bus_mode);
		bus_mode.filter = LPS28DFW_AUTO;
		bus_mode.interface = LPS28DFW_SEL_BY_HW;
		lps28dfw_bus_mode_set(ctx, &bus_mode);
	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = lps28dfw_mode_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to set odr %d", dev->name, cfg->odr);
		return ret;
	}

#ifdef CONFIG_LPS2XDF_TRIGGER
	if (cfg->trig_enabled) {
		if (lps2xdf_init_interrupt(dev, DEVICE_VARIANT_LPS28DFW) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}
