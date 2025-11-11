/* ST Microelectronics ILPS22QS pressure and temperature sensor
 *
 * Copyright (c) 2023-2024 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lps2xdf.h"
#include "ilps22qs.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LPS2XDF, CONFIG_SENSOR_LOG_LEVEL);

static inline int ilps22qs_mode_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	ilps22qs_md_t md = { 0 };

	md.odr = odr;
	md.avg = cfg->avg;
	md.lpf = cfg->lpf;
	md.fs = cfg->fs;

	return ilps22qs_mode_set(ctx, &md);
}

static int ilps22qs_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct lps2xdf_data *data = dev->data;
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	ilps22qs_data_t raw_data;
	ilps22qs_md_t md = { 0 };

	md.fs = cfg->fs;

	if (ilps22qs_data_get(ctx, &md, &raw_data) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	data->sample_press = raw_data.pressure.raw;
	data->sample_temp = raw_data.heat.raw;

	return 0;
}

#ifdef CONFIG_LPS2XDF_TRIGGER
/**
 * ilps22qs_config_interrupt - not supported
 */
static int ilps22qs_config_interrupt(const struct device *dev)
{
	return -ENOTSUP;
}

/**
 * ilps22qs_handle_interrupt - not supported
 */
static void ilps22qs_handle_interrupt(const struct device *dev)
{
}

/**
 * ilps22qs_trigger_set - not supported
 */
static int ilps22qs_trigger_set(const struct device *dev,
				const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	return -ENOTSUP;
}
#endif /* CONFIG_LPS2XDF_TRIGGER */

const struct lps2xdf_chip_api st_ilps22qs_chip_api = {
	.mode_set_odr_raw = ilps22qs_mode_set_odr_raw,
	.sample_fetch = ilps22qs_sample_fetch,
#if CONFIG_LPS2XDF_TRIGGER
	.config_interrupt = ilps22qs_config_interrupt,
	.handle_interrupt = ilps22qs_handle_interrupt,
	.trigger_set = ilps22qs_trigger_set,
#endif
};

int st_ilps22qs_init(const struct device *dev)
{
	const struct lps2xdf_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	ilps22qs_id_t id;
	ilps22qs_stat_t status;
	uint8_t tries = 10;
	int ret;

#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(st_ilps22qs, i3c)
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

	if (ilps22qs_id_get(ctx, &id) < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return -EIO;
	}

	if (id.whoami != ILPS22QS_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, id.whoami);
		return -EIO;
	}

	LOG_DBG("%s: chip id 0x%x", dev->name, id.whoami);

	/* Restore default configuration */
	if (ilps22qs_init_set(ctx, ILPS22QS_RESET) < 0) {
		LOG_ERR("%s: Not able to reset device", dev->name);
		return -EIO;
	}

	do {
		if (!--tries) {
			LOG_DBG("sw reset timed out");
			return -ETIMEDOUT;
		}
		k_usleep(LPS2XDF_SWRESET_WAIT_TIME_US);

		if (ilps22qs_status_get(ctx, &status) < 0) {
			return -EIO;
		}
	} while (status.sw_reset);

	/* Set bdu and if_inc recommended for driver usage */
	if (ilps22qs_init_set(ctx, ILPS22QS_DRV_RDY) < 0) {
		LOG_ERR("%s: Not able to set device to ready state", dev->name);
		return -EIO;
	}

	if (ON_I3C_BUS(cfg)) {
		ilps22qs_bus_mode_t bus_mode;

		/* Select bus interface */
		ilps22qs_bus_mode_get(ctx, &bus_mode);
		bus_mode.filter = ILPS22QS_FILTER_AUTO;
		bus_mode.interface = ILPS22QS_SEL_BY_HW;
		ilps22qs_bus_mode_set(ctx, &bus_mode);
	}

	/* set sensor default odr */
	LOG_DBG("%s: odr: %d", dev->name, cfg->odr);
	ret = ilps22qs_mode_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: Failed to set odr %d", dev->name, cfg->odr);
		return ret;
	}

#ifdef CONFIG_LPS2XDF_TRIGGER
	if (cfg->trig_enabled) {
		if (lps2xdf_init_interrupt(dev, DEVICE_VARIANT_ILPS22QS) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}
