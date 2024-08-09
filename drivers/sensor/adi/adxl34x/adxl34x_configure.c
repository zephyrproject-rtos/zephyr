/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor/adxl34x.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "adxl34x_reg.h"
#include "adxl34x_private.h"
#include "adxl34x_configure.h"

LOG_MODULE_DECLARE(adxl34x, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_ADXL34X_EXTENDED_API

/**
 * Fetches register 0x1D from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_thresh_tap(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	rc = config->bus_read(dev, ADXL34X_REG_THRESH_TAP, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->thresh_tap = reg_value;
	LOG_DBG("Get thresh_tap: %u", cfg->thresh_tap);
	return 0;
}

/**
 * Get register 0x1D from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] thresh_tap The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_thresh_tap(const struct device *dev, uint8_t *thresh_tap, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_thresh_tap(dev);
		if (rc != 0) {
			*thresh_tap = 0;
			return rc;
		}
	}
	*thresh_tap = data->cfg.thresh_tap;
	return 0;
}

/**
 * Set register 0x1D from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] thresh_tap The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_thresh_tap(const struct device *dev, uint8_t thresh_tap)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (thresh_tap != cfg->thresh_tap) {
		LOG_DBG("Set thresh_tap: %u", thresh_tap);
		rc = config->bus_write(dev, ADXL34X_REG_THRESH_TAP, thresh_tap);
		if (rc != 0) {
			return rc;
		}
		cfg->thresh_tap = thresh_tap;
	}
	return 0;
}

#endif /* CONFIG_ADXL34X_EXTENDED_API */

/**
 * Fetches register 0x1E from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_ofsx(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_OFSX, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->ofsx = reg_value;
	LOG_DBG("Get ofsx: %u", cfg->ofsx);
	return 0;
}

/**
 * Get register 0x1E from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] ofsx The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_ofsx(const struct device *dev, int8_t *ofsx, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_ofsx(dev);
		if (rc != 0) {
			*ofsx = 0;
			return rc;
		}
	}
	*ofsx = data->cfg.ofsx;
	return 0;
}

/**
 * Set register 0x1E from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] ofsx The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_ofsx(const struct device *dev, int8_t ofsx)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (ofsx != cfg->ofsx) {
		LOG_DBG("Set ofsx: %u", ofsx);
		rc = config->bus_write(dev, ADXL34X_REG_OFSX, ofsx);
		if (rc != 0) {
			return rc;
		}
		cfg->ofsx = ofsx;
	}
	return 0;
}

/**
 * Fetches register 0x1F from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_ofsy(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_OFSY, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->ofsy = reg_value;
	LOG_DBG("Get ofsy: %u", cfg->ofsy);
	return 0;
}

/**
 * Get register 0x1F from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] ofsy The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_ofsy(const struct device *dev, int8_t *ofsy, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_ofsy(dev);
		if (rc != 0) {
			*ofsy = 0;
			return rc;
		}
	}
	*ofsy = data->cfg.ofsy;
	return 0;
}

/**
 * Set register 0x1F from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] ofsy The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_ofsy(const struct device *dev, int8_t ofsy)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (ofsy != cfg->ofsy) {
		LOG_DBG("Set ofsy: %u", ofsy);
		rc = config->bus_write(dev, ADXL34X_REG_OFSY, ofsy);
		if (rc != 0) {
			return rc;
		}
		cfg->ofsy = ofsy;
	}
	return 0;
}

/**
 * Fetches register 0x20 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_ofsz(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_OFSZ, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->ofsz = reg_value;
	LOG_DBG("Get ofsz: %u", cfg->ofsz);
	return 0;
}

/**
 * Get register 0x20 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] ofsz The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_ofsz(const struct device *dev, int8_t *ofsz, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_ofsz(dev);
		if (rc != 0) {
			*ofsz = 0;
			return rc;
		}
	}
	*ofsz = data->cfg.ofsz;
	return 0;
}

/**
 * Set register 0x20 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] ofsz The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_ofsz(const struct device *dev, int8_t ofsz)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (ofsz != cfg->ofsz) {
		LOG_DBG("Set ofsz: %u", ofsz);
		rc = config->bus_write(dev, ADXL34X_REG_OFSZ, ofsz);
		if (rc != 0) {
			return rc;
		}
		cfg->ofsz = ofsz;
	}
	return 0;
}

#ifdef CONFIG_ADXL34X_EXTENDED_API

/**
 * Fetches register 0x21 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_dur(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_DUR, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->dur = reg_value;
	LOG_DBG("Get dur: %u", cfg->dur);
	return 0;
}

/**
 * Get register 0x21 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] dur The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_dur(const struct device *dev, uint8_t *dur, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_dur(dev);
		if (rc != 0) {
			*dur = 0;
			return rc;
		}
	}
	*dur = data->cfg.dur;
	return 0;
}

/**
 * Set register 0x21 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] dur The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_dur(const struct device *dev, uint8_t dur)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (dur != cfg->dur) {
		LOG_DBG("Set dur: %u", dur);
		rc = config->bus_write(dev, ADXL34X_REG_DUR, dur);
		if (rc != 0) {
			return rc;
		}
		cfg->dur = dur;
	}
	return 0;
}

/**
 * Fetches register 0x22 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_latent(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_LATENT, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->latent = reg_value;
	LOG_DBG("Get latent: %u", cfg->latent);
	return 0;
}

/**
 * Get register 0x22 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] latent The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_latent(const struct device *dev, uint8_t *latent, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_latent(dev);
		if (rc != 0) {
			*latent = 0;
			return rc;
		}
	}
	*latent = data->cfg.latent;
	return 0;
}

/**
 * Set register 0x22 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] latent The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_latent(const struct device *dev, uint8_t latent)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (latent != cfg->latent) {
		LOG_DBG("Set latent: %u", latent);
		rc = config->bus_write(dev, ADXL34X_REG_LATENT, latent);
		if (rc != 0) {
			return rc;
		}
		cfg->latent = latent;
	}
	return 0;
}

/**
 * Fetches register 0x23 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_window(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_WINDOW, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->window = reg_value;
	LOG_DBG("Get window: %u", cfg->window);
	return 0;
}

/**
 * Get register 0x23 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] window The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_window(const struct device *dev, uint8_t *window, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_window(dev);
		if (rc != 0) {
			*window = 0;
			return rc;
		}
	}
	*window = data->cfg.window;
	return 0;
}

/**
 * Set register 0x23 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] window The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_window(const struct device *dev, uint8_t window)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (window != cfg->window) {
		LOG_DBG("Set window: %u", window);
		rc = config->bus_write(dev, ADXL34X_REG_WINDOW, window);
		if (rc != 0) {
			return rc;
		}
		cfg->window = window;
	}
	return 0;
}

/**
 * Fetches register 0x24 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_thresh_act(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_THRESH_ACT, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->thresh_act = reg_value;
	LOG_DBG("Get thresh_act: %u", cfg->thresh_act);
	return 0;
}

/**
 * Get register 0x24 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] thresh_act The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_thresh_act(const struct device *dev, uint8_t *thresh_act, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_thresh_act(dev);
		if (rc != 0) {
			*thresh_act = 0;
			return rc;
		}
	}
	*thresh_act = data->cfg.thresh_act;
	return 0;
}

/**
 * Set register 0x24 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] thresh_act The act of thresh
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_thresh_act(const struct device *dev, uint8_t thresh_act)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (thresh_act != cfg->thresh_act) {
		LOG_DBG("Set thresh_act: %u", thresh_act);
		rc = config->bus_write(dev, ADXL34X_REG_THRESH_ACT, thresh_act);
		if (rc != 0) {
			return rc;
		}
		cfg->thresh_act = thresh_act;
	}
	return 0;
}

/**
 * Fetches register 0x25 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_thresh_inact(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_THRESH_INACT, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->thresh_inact = reg_value;
	LOG_DBG("Get thresh_inact: %u", cfg->thresh_inact);
	return 0;
}

/**
 * Get register 0x25 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] thresh_inact The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_thresh_inact(const struct device *dev, uint8_t *thresh_inact, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_thresh_inact(dev);
		if (rc != 0) {
			*thresh_inact = 0;
			return rc;
		}
	}
	*thresh_inact = data->cfg.thresh_inact;
	return 0;
}

/**
 * Set register 0x25 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] thresh_inact The inact of thresh
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_thresh_inact(const struct device *dev, uint8_t thresh_inact)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (thresh_inact != cfg->thresh_inact) {
		LOG_DBG("Set thresh_inact: %u", thresh_inact);
		rc = config->bus_write(dev, ADXL34X_REG_THRESH_INACT, thresh_inact);
		if (rc != 0) {
			return rc;
		}
		cfg->thresh_inact = thresh_inact;
	}
	return 0;
}

/**
 * Fetches register 0x26 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_time_inact(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_TIME_INACT, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->time_inact = reg_value;
	LOG_DBG("Get time_inact: %u", cfg->time_inact);
	return 0;
}

/**
 * Get register 0x26 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] time_inact The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_time_inact(const struct device *dev, uint8_t *time_inact, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_time_inact(dev);
		if (rc != 0) {
			*time_inact = 0;
			return rc;
		}
	}
	*time_inact = data->cfg.time_inact;
	return 0;
}

/**
 * Set register 0x26 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] time_inact The inact of time
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_time_inact(const struct device *dev, uint8_t time_inact)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (time_inact != cfg->time_inact) {
		LOG_DBG("Set time_inact: %u", time_inact);
		rc = config->bus_write(dev, ADXL34X_REG_TIME_INACT, time_inact);
		if (rc != 0) {
			return rc;
		}
		cfg->time_inact = time_inact;
	}
	return 0;
}

/**
 * Fetches register 0x27 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_act_inact_ctl(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_ACT_INACT_CTL, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->act_inact_ctl.act_acdc = FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_ACT_ACDC, reg_value);
	cfg->act_inact_ctl.act_x_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_ACT_X_ENABLE, reg_value);
	cfg->act_inact_ctl.act_y_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_ACT_Y_ENABLE, reg_value);
	cfg->act_inact_ctl.act_z_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_ACT_Z_ENABLE, reg_value);
	cfg->act_inact_ctl.inact_acdc = FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_INACT_ACDC, reg_value);
	cfg->act_inact_ctl.inact_x_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_INACT_X_ENABLE, reg_value);
	cfg->act_inact_ctl.inact_y_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_INACT_Y_ENABLE, reg_value);
	cfg->act_inact_ctl.inact_z_enable =
		FIELD_GET(ADXL34X_REG_ACT_INACT_CTL_INACT_Z_ENABLE, reg_value);
	LOG_DBG("Get act_inact_ctl - act_acdc:%u, act_x_enable: %u, act_y_enable: %u, "
		"act_z_enable: %u, inact_acdc: %u, inact_x_enable: %u, inact_y_enable: %u, "
		"inact_z_enable: %u",
		cfg->act_inact_ctl.act_acdc, cfg->act_inact_ctl.act_x_enable,
		cfg->act_inact_ctl.act_y_enable, cfg->act_inact_ctl.act_z_enable,
		cfg->act_inact_ctl.inact_acdc, cfg->act_inact_ctl.inact_x_enable,
		cfg->act_inact_ctl.inact_y_enable, cfg->act_inact_ctl.inact_z_enable);
	return 0;
}

/**
 * Get register 0x27 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] act_inact_ctl The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_act_inact_ctl(const struct device *dev, struct adxl34x_act_inact_ctl *act_inact_ctl,
			      bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_act_inact_ctl(dev);
		if (rc != 0) {
			*act_inact_ctl = (struct adxl34x_act_inact_ctl){0};
			return rc;
		}
	}
	*act_inact_ctl = data->cfg.act_inact_ctl;
	return 0;
}

/**
 * Set register 0x27 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] act_inact_ctl The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_act_inact_ctl(const struct device *dev, struct adxl34x_act_inact_ctl *act_inact_ctl)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (act_inact_ctl->act_acdc != cfg->act_inact_ctl.act_acdc ||
	    act_inact_ctl->act_x_enable != cfg->act_inact_ctl.act_x_enable ||
	    act_inact_ctl->act_y_enable != cfg->act_inact_ctl.act_y_enable ||
	    act_inact_ctl->act_z_enable != cfg->act_inact_ctl.act_z_enable ||
	    act_inact_ctl->inact_acdc != cfg->act_inact_ctl.inact_acdc ||
	    act_inact_ctl->inact_x_enable != cfg->act_inact_ctl.inact_x_enable ||
	    act_inact_ctl->inact_y_enable != cfg->act_inact_ctl.inact_y_enable ||
	    act_inact_ctl->inact_z_enable != cfg->act_inact_ctl.inact_z_enable) {

		reg_value =
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_ACT_ACDC, act_inact_ctl->act_acdc) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_ACT_X_ENABLE,
				   act_inact_ctl->act_x_enable) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_ACT_Y_ENABLE,
				   act_inact_ctl->act_y_enable) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_ACT_Z_ENABLE,
				   act_inact_ctl->act_z_enable) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_INACT_ACDC,
				   act_inact_ctl->inact_acdc) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_INACT_X_ENABLE,
				   act_inact_ctl->inact_x_enable) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_INACT_Y_ENABLE,
				   act_inact_ctl->inact_y_enable) |
			FIELD_PREP(ADXL34X_REG_ACT_INACT_CTL_INACT_Z_ENABLE,
				   act_inact_ctl->inact_z_enable);

		LOG_DBG("Set act_inact_ctl - act_acdc:%u, act_x_enable: %u, act_y_enable: %u, "
			"act_z_enable: %u, inact_acdc: %u, inact_x_enable: %u, inact_y_enable: %u, "
			"inact_z_enable: %u",
			act_inact_ctl->act_acdc, act_inact_ctl->act_x_enable,
			act_inact_ctl->act_y_enable, act_inact_ctl->act_z_enable,
			act_inact_ctl->inact_acdc, act_inact_ctl->inact_x_enable,
			act_inact_ctl->inact_y_enable, act_inact_ctl->inact_z_enable);
		rc = config->bus_write(dev, ADXL34X_REG_ACT_INACT_CTL, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->act_inact_ctl = *act_inact_ctl;
	}
	return 0;
}

/**
 * Fetches register 0x28 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_thresh_ff(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_THRESH_FF, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->thresh_ff = reg_value;
	LOG_DBG("Get thresh_ff: %u", cfg->thresh_ff);
	return 0;
}

/**
 * Get register 0x28 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] thresh_ff The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_thresh_ff(const struct device *dev, uint8_t *thresh_ff, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_thresh_ff(dev);
		if (rc != 0) {
			*thresh_ff = 0;
			return rc;
		}
	}
	*thresh_ff = data->cfg.thresh_ff;
	return 0;
}

/**
 * Set register 0x28 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] thresh_ff The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_thresh_ff(const struct device *dev, uint8_t thresh_ff)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (thresh_ff != cfg->thresh_ff) {
		LOG_DBG("Set thresh_ff: %u", thresh_ff);
		rc = config->bus_write(dev, ADXL34X_REG_THRESH_FF, thresh_ff);
		if (rc != 0) {
			return rc;
		}
		cfg->thresh_ff = thresh_ff;
	}
	return 0;
}

/**
 * Fetches register 0x29 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_time_ff(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_TIME_FF, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->time_ff = reg_value;
	LOG_DBG("Get time_ff: %u", cfg->time_ff);
	return 0;
}

/**
 * Get register 0x29 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] time_ff The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_time_ff(const struct device *dev, uint8_t *time_ff, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_time_ff(dev);
		if (rc != 0) {
			*time_ff = 0;
			return rc;
		}
	}
	*time_ff = data->cfg.time_ff;
	return 0;
}

/**
 * Set register 0x29 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[in] time_ff The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_time_ff(const struct device *dev, uint8_t time_ff)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;

	if (time_ff != cfg->time_ff) {
		LOG_DBG("Set time_ff: %u", time_ff);
		rc = config->bus_write(dev, ADXL34X_REG_TIME_FF, time_ff);
		if (rc != 0) {
			return rc;
		}
		cfg->time_ff = time_ff;
	}
	return 0;
}

/**
 * Fetches register 0x2A from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_tap_axes(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_TAP_AXES, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->tap_axes.improved_tab = FIELD_GET(ADXL34X_REG_TAP_AXES_IMPROVED_TAB, reg_value);
	cfg->tap_axes.suppress = FIELD_GET(ADXL34X_REG_TAP_AXES_SUPPRESS, reg_value);
	cfg->tap_axes.tap_x_enable = FIELD_GET(ADXL34X_REG_TAP_AXES_TAP_X_ENABLE, reg_value);
	cfg->tap_axes.tap_y_enable = FIELD_GET(ADXL34X_REG_TAP_AXES_TAP_Y_ENABLE, reg_value);
	cfg->tap_axes.tap_z_enable = FIELD_GET(ADXL34X_REG_TAP_AXES_TAP_Z_ENABLE, reg_value);
	LOG_DBG("Get tap_axes - improved_tab:%u, suppress:%u, tap_x_enable: %u, tap_y_enable: %u, "
		"tap_z_enable: %u",
		cfg->tap_axes.improved_tab, cfg->tap_axes.suppress, cfg->tap_axes.tap_x_enable,
		cfg->tap_axes.tap_y_enable, cfg->tap_axes.tap_z_enable);
	return 0;
}

/**
 * Get register 0x2A from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] tap_axes The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_tap_axes(const struct device *dev, struct adxl34x_tap_axes *tap_axes,
			 bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_tap_axes(dev);
		if (rc != 0) {
			*tap_axes = (struct adxl34x_tap_axes){0};
			return rc;
		}
	}
	*tap_axes = data->cfg.tap_axes;
	return 0;
}

/**
 * Set register 0x2A from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] tap_axes The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_tap_axes(const struct device *dev, struct adxl34x_tap_axes *tap_axes)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (tap_axes->improved_tab != cfg->tap_axes.improved_tab ||
	    tap_axes->suppress != cfg->tap_axes.suppress ||
	    tap_axes->tap_x_enable != cfg->tap_axes.tap_x_enable ||
	    tap_axes->tap_y_enable != cfg->tap_axes.tap_y_enable ||
	    tap_axes->tap_z_enable != cfg->tap_axes.tap_z_enable) {

		reg_value = FIELD_PREP(ADXL34X_REG_TAP_AXES_IMPROVED_TAB, tap_axes->improved_tab) |
			    FIELD_PREP(ADXL34X_REG_TAP_AXES_SUPPRESS, tap_axes->suppress) |
			    FIELD_PREP(ADXL34X_REG_TAP_AXES_TAP_X_ENABLE, tap_axes->tap_x_enable) |
			    FIELD_PREP(ADXL34X_REG_TAP_AXES_TAP_Y_ENABLE, tap_axes->tap_y_enable) |
			    FIELD_PREP(ADXL34X_REG_TAP_AXES_TAP_Z_ENABLE, tap_axes->tap_z_enable);

		LOG_DBG("Set tap_axes - improved_tab:%u, suppress:%u, tap_x_enable: %u, "
			"tap_y_enable: %u, tap_z_enable: %u",
			tap_axes->improved_tab, tap_axes->suppress, tap_axes->tap_x_enable,
			tap_axes->tap_y_enable, tap_axes->tap_z_enable);
		rc = config->bus_write(dev, ADXL34X_REG_TAP_AXES, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->tap_axes = *tap_axes;
	}
	return 0;
}

#endif /* CONFIG_ADXL34X_EXTENDED_API */

/**
 * Fetches register 0x2C from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_bw_rate(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_BW_RATE, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->bw_rate.low_power = FIELD_GET(ADXL34X_REG_BW_RATE_LOW_POWER, reg_value);
	cfg->bw_rate.rate = FIELD_GET(ADXL34X_REG_BW_RATE_RATE, reg_value);
	LOG_DBG("Get bw_rate - low_power:%u, rate: %u", cfg->bw_rate.low_power, cfg->bw_rate.rate);
	return 0;
}

/**
 * Get register 0x2C from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] bw_rate The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_bw_rate(const struct device *dev, struct adxl34x_bw_rate *bw_rate, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_bw_rate(dev);
		if (rc != 0) {
			*bw_rate = (struct adxl34x_bw_rate){0};
			return rc;
		}
	}
	*bw_rate = data->cfg.bw_rate;
	return 0;
}

/**
 * Set register 0x2C from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] bw_rate The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_bw_rate(const struct device *dev, struct adxl34x_bw_rate *bw_rate)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (bw_rate->low_power != cfg->bw_rate.low_power || bw_rate->rate != cfg->bw_rate.rate) {
		reg_value = FIELD_PREP(ADXL34X_REG_BW_RATE_LOW_POWER, bw_rate->low_power) |
			    FIELD_PREP(ADXL34X_REG_BW_RATE_RATE, bw_rate->rate);

		LOG_DBG("Set bw_rate - low_power:%u, rate: %u", bw_rate->low_power, bw_rate->rate);
		rc = config->bus_write(dev, ADXL34X_REG_BW_RATE, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->bw_rate = *bw_rate;
	}
	return 0;
}

/**
 * Fetches register 0x2D from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_power_ctl(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_POWER_CTL, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->power_ctl.link = FIELD_GET(ADXL34X_REG_POWER_CTL_LINK, reg_value);
	cfg->power_ctl.auto_sleep = FIELD_GET(ADXL34X_REG_POWER_CTL_AUTO_SLEEP, reg_value);
	cfg->power_ctl.measure = FIELD_GET(ADXL34X_REG_POWER_CTL_MEASURE, reg_value);
	cfg->power_ctl.sleep = FIELD_GET(ADXL34X_REG_POWER_CTL_SLEEP, reg_value);
	cfg->power_ctl.wakeup = FIELD_GET(ADXL34X_REG_POWER_CTL_WAKEUP, reg_value);
	LOG_DBG("Get power_ctl - link:%u, auto_sleep: %u, measure: %u, sleep: %u, wakeup: "
		"%u",
		cfg->power_ctl.link, cfg->power_ctl.auto_sleep, cfg->power_ctl.measure,
		cfg->power_ctl.sleep, cfg->power_ctl.wakeup);
	return 0;
}

/**
 * Get register 0x2D from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] power_ctl The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_power_ctl(const struct device *dev, struct adxl34x_power_ctl *power_ctl,
			  bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_power_ctl(dev);
		if (rc != 0) {
			*power_ctl = (struct adxl34x_power_ctl){0};
			return rc;
		}
	}
	*power_ctl = data->cfg.power_ctl;
	return 0;
}

/**
 * Set register 0x2D from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] power_ctl The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_power_ctl(const struct device *dev, struct adxl34x_power_ctl *power_ctl)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (power_ctl->link != cfg->power_ctl.link ||
	    power_ctl->auto_sleep != cfg->power_ctl.auto_sleep ||
	    power_ctl->measure != cfg->power_ctl.measure ||
	    power_ctl->sleep != cfg->power_ctl.sleep ||
	    power_ctl->wakeup != cfg->power_ctl.wakeup) {

		reg_value = FIELD_PREP(ADXL34X_REG_POWER_CTL_LINK, power_ctl->link) |
			    FIELD_PREP(ADXL34X_REG_POWER_CTL_AUTO_SLEEP, power_ctl->auto_sleep) |
			    FIELD_PREP(ADXL34X_REG_POWER_CTL_MEASURE, power_ctl->measure) |
			    FIELD_PREP(ADXL34X_REG_POWER_CTL_SLEEP, power_ctl->sleep) |
			    FIELD_PREP(ADXL34X_REG_POWER_CTL_WAKEUP, power_ctl->wakeup);

		LOG_DBG("Set power_ctl - link:%u, auto_sleep: %u, measure: %u, sleep: %u, wakeup: "
			"%u",
			power_ctl->link, power_ctl->auto_sleep, power_ctl->measure,
			power_ctl->sleep, power_ctl->wakeup);
		rc = config->bus_write(dev, ADXL34X_REG_POWER_CTL, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->power_ctl = *power_ctl;
	}
	return 0;
}

/**
 * Fetches register 0x2E from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_int_enable(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_INT_ENABLE, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->int_enable.data_ready = FIELD_GET(ADXL34X_REG_INT_ENABLE_DATA_READY, reg_value);
	cfg->int_enable.single_tap = FIELD_GET(ADXL34X_REG_INT_ENABLE_SINGLE_TAP, reg_value);
	cfg->int_enable.double_tap = FIELD_GET(ADXL34X_REG_INT_ENABLE_DOUBLE_TAP, reg_value);
	cfg->int_enable.activity = FIELD_GET(ADXL34X_REG_INT_ENABLE_ACTIVITY, reg_value);
	cfg->int_enable.inactivity = FIELD_GET(ADXL34X_REG_INT_ENABLE_INACTIVITY, reg_value);
	cfg->int_enable.free_fall = FIELD_GET(ADXL34X_REG_INT_ENABLE_FREE_FALL, reg_value);
	cfg->int_enable.watermark = FIELD_GET(ADXL34X_REG_INT_ENABLE_WATERMARK, reg_value);
	cfg->int_enable.overrun = FIELD_GET(ADXL34X_REG_INT_ENABLE_OVERRUN, reg_value);
	LOG_DBG("Get int_enable - data_ready:%u, single_tap: %u, double_tap: %u, activity: "
		"%u, inactivity: %u, free_fall: %u, watermark: %u, overrun: %u",
		cfg->int_enable.data_ready, cfg->int_enable.single_tap, cfg->int_enable.double_tap,
		cfg->int_enable.activity, cfg->int_enable.inactivity, cfg->int_enable.free_fall,
		cfg->int_enable.watermark, cfg->int_enable.overrun);
	return 0;
}

/**
 * Get register 0x2E from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] int_enable The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_int_enable(const struct device *dev, struct adxl34x_int_enable *int_enable,
			   bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_int_enable(dev);
		if (rc != 0) {
			*int_enable = (struct adxl34x_int_enable){0};
			return rc;
		}
	}
	*int_enable = data->cfg.int_enable;
	return 0;
}

/**
 * Set register 0x2E from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] int_enable The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_int_enable(const struct device *dev, struct adxl34x_int_enable *int_enable)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (int_enable->data_ready != cfg->int_enable.data_ready ||
	    int_enable->single_tap != cfg->int_enable.single_tap ||
	    int_enable->double_tap != cfg->int_enable.double_tap ||
	    int_enable->activity != cfg->int_enable.activity ||
	    int_enable->inactivity != cfg->int_enable.inactivity ||
	    int_enable->free_fall != cfg->int_enable.free_fall ||
	    int_enable->watermark != cfg->int_enable.watermark ||
	    int_enable->overrun != cfg->int_enable.overrun) {

		reg_value = FIELD_PREP(ADXL34X_REG_INT_ENABLE_DATA_READY, int_enable->data_ready) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_SINGLE_TAP, int_enable->single_tap) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_DOUBLE_TAP, int_enable->double_tap) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_ACTIVITY, int_enable->activity) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_INACTIVITY, int_enable->inactivity) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_FREE_FALL, int_enable->free_fall) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_WATERMARK, int_enable->watermark) |
			    FIELD_PREP(ADXL34X_REG_INT_ENABLE_OVERRUN, int_enable->overrun);

		LOG_DBG("Set int_enable - data_ready:%u, single_tap: %u, double_tap: %u, activity: "
			"%u, inactivity: %u, free_fall: %u, watermark: %u, overrun: %u",
			int_enable->data_ready, int_enable->single_tap, int_enable->double_tap,
			int_enable->activity, int_enable->inactivity, int_enable->free_fall,
			int_enable->watermark, int_enable->overrun);
		rc = config->bus_write(dev, ADXL34X_REG_INT_ENABLE, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->int_enable = *int_enable;
	}
	return 0;
}

/**
 * Fetches register 0x2F from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_int_map(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_INT_MAP, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->int_map.data_ready = FIELD_GET(ADXL34X_REG_INT_MAP_DATA_READY, reg_value);
	cfg->int_map.single_tap = FIELD_GET(ADXL34X_REG_INT_MAP_SINGLE_TAP, reg_value);
	cfg->int_map.double_tap = FIELD_GET(ADXL34X_REG_INT_MAP_DOUBLE_TAP, reg_value);
	cfg->int_map.activity = FIELD_GET(ADXL34X_REG_INT_MAP_ACTIVITY, reg_value);
	cfg->int_map.inactivity = FIELD_GET(ADXL34X_REG_INT_MAP_INACTIVITY, reg_value);
	cfg->int_map.free_fall = FIELD_GET(ADXL34X_REG_INT_MAP_FREE_FALL, reg_value);
	cfg->int_map.watermark = FIELD_GET(ADXL34X_REG_INT_MAP_WATERMARK, reg_value);
	cfg->int_map.overrun = FIELD_GET(ADXL34X_REG_INT_MAP_OVERRUN, reg_value);
	LOG_DBG("Get int_map - data_ready:%u, single_tap: %u, double_tap: %u, activity: %u, "
		"inactivity: %u, free_fall: %u, watermark: %u, overrun: %u",
		cfg->int_map.data_ready, cfg->int_map.single_tap, cfg->int_map.double_tap,
		cfg->int_map.activity, cfg->int_map.inactivity, cfg->int_map.free_fall,
		cfg->int_map.watermark, cfg->int_map.overrun);
	return 0;
}

/**
 * Get register 0x2F from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] int_map The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_int_map(const struct device *dev, struct adxl34x_int_map *int_map, bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_int_map(dev);
		if (rc != 0) {
			*int_map = (struct adxl34x_int_map){0};
			return rc;
		}
	}
	*int_map = data->cfg.int_map;
	return 0;
}

/**
 * Set register 0x2F from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] int_map The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_int_map(const struct device *dev, struct adxl34x_int_map *int_map)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (int_map->data_ready != cfg->int_map.data_ready ||
	    int_map->single_tap != cfg->int_map.single_tap ||
	    int_map->double_tap != cfg->int_map.double_tap ||
	    int_map->activity != cfg->int_map.activity ||
	    int_map->inactivity != cfg->int_map.inactivity ||
	    int_map->free_fall != cfg->int_map.free_fall ||
	    int_map->watermark != cfg->int_map.watermark ||
	    int_map->overrun != cfg->int_map.overrun) {

		reg_value = FIELD_PREP(ADXL34X_REG_INT_MAP_DATA_READY, int_map->data_ready) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_SINGLE_TAP, int_map->single_tap) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_DOUBLE_TAP, int_map->double_tap) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_ACTIVITY, int_map->activity) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_INACTIVITY, int_map->inactivity) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_FREE_FALL, int_map->free_fall) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_WATERMARK, int_map->watermark) |
			    FIELD_PREP(ADXL34X_REG_INT_MAP_OVERRUN, int_map->overrun);

		LOG_DBG("Set int_map - data_ready:%u, single_tap: %u, double_tap: %u, activity: "
			"%u, "
			"inactivity: %u, free_fall: %u, watermark: %u, overrun: %u",
			int_map->data_ready, int_map->single_tap, int_map->double_tap,
			int_map->activity, int_map->inactivity, int_map->free_fall,
			int_map->watermark, int_map->overrun);
		rc = config->bus_write(dev, ADXL34X_REG_INT_MAP, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->int_map = *int_map;
	}
	return 0;
}

/**
 * Fetches register 0x31 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_data_format(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_DATA_FORMAT, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->data_format.self_test = FIELD_GET(ADXL34X_REG_DATA_FORMAT_SELF_TEST, reg_value);
	cfg->data_format.spi = FIELD_GET(ADXL34X_REG_DATA_FORMAT_SPI, reg_value);
	cfg->data_format.int_invert = FIELD_GET(ADXL34X_REG_DATA_FORMAT_INT_INVERT, reg_value);
	cfg->data_format.full_res = FIELD_GET(ADXL34X_REG_DATA_FORMAT_FULL_RES, reg_value);
	cfg->data_format.justify = FIELD_GET(ADXL34X_REG_DATA_FORMAT_JUSTIFY, reg_value);
	cfg->data_format.range = FIELD_GET(ADXL34X_REG_DATA_FORMAT_RANGE, reg_value);
	LOG_DBG("Get data_format - self_test:%u, spi: %u, int_invert: %u, full_res: %u, "
		"justify: %u, range: %u",
		cfg->data_format.self_test, cfg->data_format.spi, cfg->data_format.int_invert,
		cfg->data_format.full_res, cfg->data_format.justify, cfg->data_format.range);
	return 0;
}

/**
 * Get register 0x31 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] data_format The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_data_format(const struct device *dev, struct adxl34x_data_format *data_format,
			    bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_data_format(dev);
		if (rc != 0) {
			*data_format = (struct adxl34x_data_format){0};
			return rc;
		}
	}
	*data_format = data->cfg.data_format;
	return 0;
}

/**
 * Set register 0x31 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] data_format The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_data_format(const struct device *dev, struct adxl34x_data_format *data_format)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (data_format->self_test != cfg->data_format.self_test ||
	    data_format->spi != cfg->data_format.spi ||
	    data_format->int_invert != cfg->data_format.int_invert ||
	    data_format->full_res != cfg->data_format.full_res ||
	    data_format->justify != cfg->data_format.justify ||
	    data_format->range != cfg->data_format.range) {

		reg_value =
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_SELF_TEST, data_format->self_test) |
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_SPI, data_format->spi) |
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_INT_INVERT, data_format->int_invert) |
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_FULL_RES, data_format->full_res) |
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_JUSTIFY, data_format->justify) |
			FIELD_PREP(ADXL34X_REG_DATA_FORMAT_RANGE, data_format->range);

		LOG_DBG("Set data_format - self_test:%u, spi: %u, int_invert: %u, full_res: %u, "
			"justify: %u, range: %u",
			data_format->self_test, data_format->spi, data_format->int_invert,
			data_format->full_res, data_format->justify, data_format->range);
		rc = config->bus_write(dev, ADXL34X_REG_DATA_FORMAT, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->data_format = *data_format;
	}
	return 0;
}

/**
 * Fetches register 0x38 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_fifo_ctl(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_FIFO_CTL, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->fifo_ctl.fifo_mode = FIELD_GET(ADXL34X_REG_FIFO_CTL_FIFO_MODE, reg_value);
	cfg->fifo_ctl.trigger = FIELD_GET(ADXL34X_REG_FIFO_CTL_TRIGGER, reg_value);
	cfg->fifo_ctl.samples = FIELD_GET(ADXL34X_REG_FIFO_CTL_SAMPLES, reg_value);
	LOG_DBG("Get fifo_ctl - fifo_mode:%u, trigger: %u, samples: %u", cfg->fifo_ctl.fifo_mode,
		cfg->fifo_ctl.trigger, cfg->fifo_ctl.samples);
	return 0;
}

/**
 * Get register 0x38 from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] fifo_ctl The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_fifo_ctl(const struct device *dev, struct adxl34x_fifo_ctl *fifo_ctl,
			 bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (!use_cache) {
		rc = adxl34x_load_fifo_ctl(dev);
		if (rc != 0) {
			*fifo_ctl = (struct adxl34x_fifo_ctl){0};
			return rc;
		}
	}
	*fifo_ctl = data->cfg.fifo_ctl;
	return 0;
}

/**
 * Set register 0x38 from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] fifo_ctl The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_fifo_ctl(const struct device *dev, struct adxl34x_fifo_ctl *fifo_ctl)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (fifo_ctl->fifo_mode != cfg->fifo_ctl.fifo_mode ||
	    fifo_ctl->trigger != cfg->fifo_ctl.trigger ||
	    fifo_ctl->samples != cfg->fifo_ctl.samples) {

		reg_value = FIELD_PREP(ADXL34X_REG_FIFO_CTL_FIFO_MODE, fifo_ctl->fifo_mode) |
			    FIELD_PREP(ADXL34X_REG_FIFO_CTL_TRIGGER, fifo_ctl->trigger) |
			    FIELD_PREP(ADXL34X_REG_FIFO_CTL_SAMPLES, fifo_ctl->samples);

		LOG_DBG("Set fifo_ctl - fifo_mode:%u, trigger: %u, samples: %u",
			fifo_ctl->fifo_mode, fifo_ctl->trigger, fifo_ctl->samples);
		rc = config->bus_write(dev, ADXL34X_REG_FIFO_CTL, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->fifo_ctl = *fifo_ctl;
	}
	return 0;
}

#ifdef CONFIG_ADXL34X_EXTENDED_API

/**
 * Fetches register 0x3B from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_orient_conf(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_ORIENT_CONF, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->orient_conf.int_orient = FIELD_GET(ADXL34X_REG_ORIENT_CONF_INT_ORIENT, reg_value);
	cfg->orient_conf.dead_zone = FIELD_GET(ADXL34X_REG_ORIENT_CONF_DEAD_ZONE, reg_value);
	cfg->orient_conf.int_3d = FIELD_GET(ADXL34X_REG_ORIENT_CONF_INT_3D, reg_value);
	cfg->orient_conf.divisor = FIELD_GET(ADXL34X_REG_ORIENT_CONF_DIVISOR, reg_value);
	LOG_DBG("Get orient_conf - int_orient:%u, dead_zone: %u, int_3d: %u, divisor: %u",
		cfg->orient_conf.int_orient, cfg->orient_conf.dead_zone, cfg->orient_conf.int_3d,
		cfg->orient_conf.divisor);
	return 0;
}

/**
 * Get register 0x3B from the adxl34x, use cached values if needed
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] orient_conf The value of the register
 * @param[in] use_cache Do not fetch the register, used the cached value
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_orient_conf(const struct device *dev, struct adxl34x_orient_conf *orient_conf,
			    bool use_cache)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (data->cfg.devid != ADXL344_DEVID && data->cfg.devid != ADXL346_DEVID) {
		return -EADDRNOTAVAIL;
	}

	if (!use_cache) {
		rc = adxl34x_load_orient_conf(dev);
		if (rc != 0) {
			*orient_conf = (struct adxl34x_orient_conf){0};
			return rc;
		}
	}
	*orient_conf = data->cfg.orient_conf;
	return 0;
}

/**
 * Set register 0x3B from the adxl34x, update cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] orient_conf The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_set_orient_conf(const struct device *dev, struct adxl34x_orient_conf *orient_conf)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	int rc;
	uint8_t reg_value;

	if (data->cfg.devid != ADXL344_DEVID && data->cfg.devid != ADXL346_DEVID) {
		return -EADDRNOTAVAIL;
	}

	if (orient_conf->int_orient != cfg->orient_conf.int_orient ||
	    orient_conf->dead_zone != cfg->orient_conf.dead_zone ||
	    orient_conf->int_3d != cfg->orient_conf.int_3d ||
	    orient_conf->divisor != cfg->orient_conf.divisor) {

		reg_value =
			FIELD_PREP(ADXL34X_REG_ORIENT_CONF_INT_ORIENT, orient_conf->int_orient) |
			FIELD_PREP(ADXL34X_REG_ORIENT_CONF_DEAD_ZONE, orient_conf->dead_zone) |
			FIELD_PREP(ADXL34X_REG_ORIENT_CONF_INT_3D, orient_conf->int_3d) |
			FIELD_PREP(ADXL34X_REG_ORIENT_CONF_DIVISOR, orient_conf->divisor);

		LOG_DBG("Set orient_conf - int_orient:%u, dead_zone: %u, int_3d: %u, divisor: %u",
			orient_conf->int_orient, orient_conf->dead_zone, orient_conf->int_3d,
			orient_conf->divisor);
		rc = config->bus_write(dev, ADXL34X_REG_ORIENT_CONF, reg_value);
		if (rc != 0) {
			return rc;
		}
		cfg->orient_conf = *orient_conf;
	}
	return 0;
}

#endif /* CONFIG_ADXL34X_EXTENDED_API */

/* Read only registers */

/**
 * Get register 0x00 from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
static int adxl34x_load_devid(const struct device *dev)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	struct adxl34x_cfg *cfg = &data->cfg;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_DEVID, &reg_value);
	if (rc != 0) {
		return rc;
	}

	cfg->devid = reg_value;
	LOG_DBG("Get devid: 0x%02X", cfg->devid);
	return 0;
}

/**
 * Get register 0x00 from the adxl34x, only fetch the value if not cached, update the cached value
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] devid The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_devid(const struct device *dev, uint8_t *devid)
{
	struct adxl34x_dev_data *data = dev->data;
	int rc;

	if (data->cfg.devid == 0) {
		rc = adxl34x_load_devid(dev);
		if (rc != 0) {
			*devid = 0;
			return rc;
		}
	}
	*devid = data->cfg.devid;
	return 0;
}

#ifdef CONFIG_ADXL34X_EXTENDED_API

/**
 * Get register 0x from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] act_tap_status The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_act_tap_status(const struct device *dev,
			       struct adxl34x_act_tap_status *act_tap_status)
{
	const struct adxl34x_dev_config *config = dev->config;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_ACT_TAP_STATUS, &reg_value);
	if (rc != 0) {
		*act_tap_status = (struct adxl34x_act_tap_status){0};
		return rc;
	}

	act_tap_status->act_x_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_ACT_X_SOURCE, reg_value);
	act_tap_status->act_y_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_ACT_Y_SOURCE, reg_value);
	act_tap_status->act_z_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_ACT_Z_SOURCE, reg_value);
	act_tap_status->asleep = FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_ASLEEP, reg_value);
	act_tap_status->tap_x_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_TAP_X_SOURCE, reg_value);
	act_tap_status->tap_y_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_TAP_Y_SOURCE, reg_value);
	act_tap_status->tap_z_source =
		FIELD_GET(ADXL34X_REG_ACT_TAP_STATUS_TAP_Z_SOURCE, reg_value);
	LOG_DBG("Get act_tap_status - act_x_source:%u, act_y_source:%u, act_z_source:%u, "
		"asleep:%u, tap_x_source:%u, tap_y_source:%u, tap_z_source:%u",
		act_tap_status->act_x_source, act_tap_status->act_y_source,
		act_tap_status->act_z_source, act_tap_status->asleep, act_tap_status->tap_x_source,
		act_tap_status->tap_y_source, act_tap_status->tap_z_source);
	return 0;
}

#endif /* CONFIG_ADXL34X_EXTENDED_API */

/**
 * Get register 0x from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] int_source The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_int_source(const struct device *dev, struct adxl34x_int_source *int_source)
{
	const struct adxl34x_dev_config *config = dev->config;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_INT_SOURCE, &reg_value);
	if (rc != 0) {
		*int_source = (struct adxl34x_int_source){0};
		return rc;
	}

	int_source->data_ready = FIELD_GET(ADXL34X_REG_INT_SOURCE_DATA_READY, reg_value);
	int_source->single_tap = FIELD_GET(ADXL34X_REG_INT_SOURCE_SINGLE_TAP, reg_value);
	int_source->double_tap = FIELD_GET(ADXL34X_REG_INT_SOURCE_DOUBLE_TAP, reg_value);
	int_source->activity = FIELD_GET(ADXL34X_REG_INT_SOURCE_ACTIVITY, reg_value);
	int_source->inactivity = FIELD_GET(ADXL34X_REG_INT_SOURCE_INACTIVITY, reg_value);
	int_source->free_fall = FIELD_GET(ADXL34X_REG_INT_SOURCE_FREE_FALL, reg_value);
	int_source->watermark = FIELD_GET(ADXL34X_REG_INT_SOURCE_WATERMARK, reg_value);
	int_source->overrun = FIELD_GET(ADXL34X_REG_INT_SOURCE_OVERRUN, reg_value);
	return 0;
}

/**
 * Get register 0x from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] fifo_status The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_fifo_status(const struct device *dev, struct adxl34x_fifo_status *fifo_status)
{
	const struct adxl34x_dev_config *config = dev->config;
	uint8_t reg_value;
	int rc;

	rc = config->bus_read(dev, ADXL34X_REG_FIFO_STATUS, &reg_value);
	if (rc != 0) {
		*fifo_status = (struct adxl34x_fifo_status){0};
		return rc;
	}

	fifo_status->fifo_trig = FIELD_GET(ADXL34X_REG_FIFO_STATUS_FIFO_TRIG, reg_value);
	fifo_status->entries = FIELD_GET(ADXL34X_REG_FIFO_STATUS_ENTRIES, reg_value);
	return 0;
}

#ifdef CONFIG_ADXL34X_EXTENDED_API

/**
 * Get register 0x from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] tap_sign The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_tap_sign(const struct device *dev, struct adxl34x_tap_sign *tap_sign)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	uint8_t reg_value;
	int rc;

	if (data->cfg.devid != ADXL344_DEVID && data->cfg.devid != ADXL346_DEVID) {
		return -EADDRNOTAVAIL;
	}

	rc = config->bus_read(dev, ADXL34X_REG_TAP_SIGN, &reg_value);
	if (rc != 0) {
		*tap_sign = (struct adxl34x_tap_sign){0};
		return rc;
	}

	tap_sign->xsign = FIELD_GET(ADXL34X_REG_TAP_SIGN_XSIGN, reg_value);
	tap_sign->ysign = FIELD_GET(ADXL34X_REG_TAP_SIGN_YSIGN, reg_value);
	tap_sign->zsign = FIELD_GET(ADXL34X_REG_TAP_SIGN_ZSIGN, reg_value);
	tap_sign->xtap = FIELD_GET(ADXL34X_REG_TAP_SIGN_XTAP, reg_value);
	tap_sign->ytap = FIELD_GET(ADXL34X_REG_TAP_SIGN_YTAP, reg_value);
	tap_sign->ztap = FIELD_GET(ADXL34X_REG_TAP_SIGN_ZTAP, reg_value);
	LOG_DBG("Get tap_sign - xsign:%u, ysign:%u, zsign:%u, xtap:%u, ytap:%u, ztap:%u",
		tap_sign->xsign, tap_sign->ysign, tap_sign->zsign, tap_sign->xtap, tap_sign->ytap,
		tap_sign->ztap);
	return 0;
}

/**
 * Get register 0x from the adxl34x
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] orient The value of the register
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_orient(const struct device *dev, struct adxl34x_orient *orient)
{
	const struct adxl34x_dev_config *config = dev->config;
	struct adxl34x_dev_data *data = dev->data;
	uint8_t reg_value;
	int rc;

	if (data->cfg.devid != ADXL344_DEVID && data->cfg.devid != ADXL346_DEVID) {
		return -EADDRNOTAVAIL;
	}

	rc = config->bus_read(dev, ADXL34X_REG_ORIENT, &reg_value);
	if (rc != 0) {
		*orient = (struct adxl34x_orient){0};
		return rc;
	}

	orient->v2 = FIELD_GET(ADXL34X_REG_ORIENT_V2, reg_value);
	orient->orient_2d = FIELD_GET(ADXL34X_REG_ORIENT_2D_ORIENT, reg_value);
	orient->v3 = FIELD_GET(ADXL34X_REG_ORIENT_V3, reg_value);
	orient->orient_3d = FIELD_GET(ADXL34X_REG_ORIENT_3D_ORIENT, reg_value);
	LOG_DBG("Get orient - v2:%u, orient_2d:%u, v3:%u, orient_3d:%u", orient->v2,
		orient->orient_2d, orient->v3, orient->orient_3d);
	return 0;
}

#endif /* CONFIG_ADXL34X_EXTENDED_API */

/**
 * Update the registry of the adxl34x with the new configuration
 *
 * @param[in] dev Pointer to the sensor device
 * @param[out] new_cfg The new configuration of the registry
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_configure(const struct device *dev, struct adxl34x_cfg *new_cfg)
{
	int rc = 0;

#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_set_thresh_tap(dev, new_cfg->thresh_tap);
#endif /* CONFIG_ADXL34X_EXTENDED_API */
	rc |= adxl34x_set_ofsx(dev, new_cfg->ofsx);
	rc |= adxl34x_set_ofsy(dev, new_cfg->ofsy);
	rc |= adxl34x_set_ofsz(dev, new_cfg->ofsz);
#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_set_dur(dev, new_cfg->dur);
	rc |= adxl34x_set_latent(dev, new_cfg->latent);
	rc |= adxl34x_set_window(dev, new_cfg->window);
	rc |= adxl34x_set_thresh_act(dev, new_cfg->thresh_act);
	rc |= adxl34x_set_thresh_inact(dev, new_cfg->thresh_inact);
	rc |= adxl34x_set_time_inact(dev, new_cfg->time_inact);
	rc |= adxl34x_set_act_inact_ctl(dev, &new_cfg->act_inact_ctl);
	rc |= adxl34x_set_thresh_ff(dev, new_cfg->thresh_ff);
	rc |= adxl34x_set_time_ff(dev, new_cfg->time_ff);
	rc |= adxl34x_set_tap_axes(dev, &new_cfg->tap_axes);
#endif /* CONFIG_ADXL34X_EXTENDED_API */
	rc |= adxl34x_set_bw_rate(dev, &new_cfg->bw_rate);
	rc |= adxl34x_set_power_ctl(dev, &new_cfg->power_ctl);
	rc |= adxl34x_set_int_enable(dev, &new_cfg->int_enable);
	rc |= adxl34x_set_int_map(dev, &new_cfg->int_map);
	rc |= adxl34x_set_data_format(dev, &new_cfg->data_format);
#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_set_fifo_ctl(dev, &new_cfg->fifo_ctl);

	struct adxl34x_dev_data *data = dev->data;

	if (data->cfg.devid == ADXL344_DEVID || data->cfg.devid == ADXL346_DEVID) {
		rc |= adxl34x_set_orient_conf(dev, &new_cfg->orient_conf);
	}
#endif /* CONFIG_ADXL34X_EXTENDED_API */

	return rc;
}

/**
 * Fetch the registry of the adxl34x to update the cached values
 *
 * @param[in] dev Pointer to the sensor device
 * @return 0 if successful, negative errno code if failure.
 */
int adxl34x_get_configuration(const struct device *dev)
{
	int rc = 0;

#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_load_thresh_tap(dev);
#endif /* CONFIG_ADXL34X_EXTENDED_API */
	rc |= adxl34x_load_ofsx(dev);
	rc |= adxl34x_load_ofsy(dev);
	rc |= adxl34x_load_ofsz(dev);
#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_load_dur(dev);
	rc |= adxl34x_load_latent(dev);
	rc |= adxl34x_load_window(dev);
	rc |= adxl34x_load_thresh_act(dev);
	rc |= adxl34x_load_thresh_inact(dev);
	rc |= adxl34x_load_time_inact(dev);
	rc |= adxl34x_load_act_inact_ctl(dev);
	rc |= adxl34x_load_thresh_ff(dev);
	rc |= adxl34x_load_time_ff(dev);
	rc |= adxl34x_load_tap_axes(dev);
#endif /* CONFIG_ADXL34X_EXTENDED_API */
	rc |= adxl34x_load_bw_rate(dev);
	rc |= adxl34x_load_power_ctl(dev);
	rc |= adxl34x_load_int_enable(dev);
	rc |= adxl34x_load_int_map(dev);
	rc |= adxl34x_load_data_format(dev);
#ifdef CONFIG_ADXL34X_EXTENDED_API
	rc |= adxl34x_load_fifo_ctl(dev);

	struct adxl34x_dev_data *data = dev->data;

	if (data->cfg.devid == ADXL344_DEVID || data->cfg.devid == ADXL346_DEVID) {
		rc |= adxl34x_load_orient_conf(dev);
	}
#endif /* CONFIG_ADXL34X_EXTENDED_API */

	return rc;
}
