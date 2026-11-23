/* ST Microelectronics MIS2DU12 STdC HAL wrapper
 *
 * Copyright (c) 2026 Dotcom IoT LLP
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Milan Pipaliya <milan.pipaliya@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "lis2du12.h"

LOG_MODULE_DECLARE(LIS2DU12, CONFIG_SENSOR_LOG_LEVEL);

static int mis2du12_api_set_fs_raw(const struct device *dev, uint8_t fs)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_md_t mode;

	if (lis2du12_mode_get(ctx, &mode) < 0) {
		return -EIO;
	}

	mode.fs = fs;
	if (lis2du12_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

	return 0;
}

static int mis2du12_api_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_md_t mode;

	if (lis2du12_mode_get(ctx, &mode) < 0) {
		return -EIO;
	}

	mode.odr = odr;
	if (lis2du12_mode_set(ctx, &mode) < 0) {
		return -EIO;
	}

	return 0;
}

static int mis2du12_api_reboot(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_status_t status;
	uint8_t tries = 10;

	if (mis2du12_api_set_odr_raw(dev, LIS2DU12_OFF) < 0) {
		return -EIO;
	}

	if (lis2du12_init_set(ctx, LIS2DU12_RESET) < 0) {
		return -EIO;
	}

	do {
		if (!--tries) {
			LOG_ERR("sw reset timed out");
			return -ETIMEDOUT;
		}
		k_usleep(50);

		if (lis2du12_status_get(ctx, &status) < 0) {
			return -EIO;
		}
	} while (status.sw_reset != 0);

	if (lis2du12_init_set(ctx, LIS2DU12_DRV_RDY) < 0) {
		return -EIO;
	}

	return 0;
}

static int mis2du12_api_check_id(const struct device *dev)
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_id_t chip_id;

	if (lis2du12_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%x", chip_id.whoami);

	if (chip_id.whoami != LIS2DU12_ID) {
		LOG_ERR("Invalid chip id 0x%x (expected 0x%x)", chip_id.whoami, LIS2DU12_ID);
		return -EIO;
	}

	return 0;
}

static int mis2du12_api_read_raw(const struct device *dev, int16_t raw[3])
{
	const struct lis2du12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2du12_data_t xl_data;
	lis2du12_md_t md;

	md.fs = cfg->accel_range;
	if (lis2du12_data_get(ctx, &md, &xl_data) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	raw[0] = xl_data.xl.raw[0];
	raw[1] = xl_data.xl.raw[1];
	raw[2] = xl_data.xl.raw[2];

	return 0;
}

const struct lis2du12_chip_api mis2du12_api = {
	.set_fs_raw = mis2du12_api_set_fs_raw,
	.set_odr_raw = mis2du12_api_set_odr_raw,
	.reboot = mis2du12_api_reboot,
	.check_id = mis2du12_api_check_id,
	.read_raw = mis2du12_api_read_raw,
};
