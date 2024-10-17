/* ST Microelectronics LIS2DUX12 smart accelerometer APIs
 *
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lis2dux12.h"
#include "lis2dux12_api.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(LIS2DUX12, CONFIG_SENSOR_LOG_LEVEL);

static int32_t st_lis2dux12_set_odr_raw(const struct device *dev, uint8_t odr)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_md_t mode = {.odr = odr, .fs = data->range};

	data->odr = odr;
	return lis2dux12_mode_set(ctx, &mode);
}

static int32_t st_lis2dux12_set_range(const struct device *dev, uint8_t range)
{
	int err;
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_md_t val = { .odr = data->odr, .fs = range };

	err = lis2dux12_mode_set(ctx, &val);
	if (err) {
		return err;
	}

	switch (range) {
	case LIS2DUX12_DT_FS_2G:
		data->gain = lis2dux12_from_fs2g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_4G:
		data->gain = lis2dux12_from_fs4g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_8G:
		data->gain = lis2dux12_from_fs8g_to_mg(1);
		break;
	case LIS2DUX12_DT_FS_16G:
		data->gain = lis2dux12_from_fs16g_to_mg(1);
		break;
	default:
		LOG_ERR("range %d not supported.", range);
		return -EINVAL;
	}

	data->range = range;
	return 0;
}

static int32_t st_lis2dux12_sample_fetch_accel(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	/* fetch raw data sample */
	lis2dux12_md_t mode = {.fs = data->range};
	lis2dux12_xl_data_t xzy_data = {0};

	if (lis2dux12_xl_data_get(ctx, &mode, &xzy_data) < 0) {
		LOG_ERR("Failed to fetch raw data sample");
		return -EIO;
	}

	data->sample_x = xzy_data.raw[0];
	data->sample_y = xzy_data.raw[1];
	data->sample_z = xzy_data.raw[2];

	return 0;
}

#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
static int32_t st_lis2dux12_sample_fetch_temp(const struct device *dev)
{
	struct lis2dux12_data *data = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	/* fetch raw data sample */
	lis2dux12_outt_data_t temp_data = {0};

	if (lis2dux12_outt_data_get(ctx, &temp_data) < 0) {
		LOG_ERR("Failed to fetch raw temperature data sample");
		return -EIO;
	}

	data->sample_temp = temp_data.heat.deg_c;

	return 0;
}
#endif

#ifdef CONFIG_LIS2DUX12_TRIGGER
static void st_lis2dux12_handle_interrupt(const struct device *dev)
{
	struct lis2dux12_data *lis2dux12 = dev->data;
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_all_sources_t sources;
	int ret;

	lis2dux12_all_sources_get(ctx, &sources);

	if (sources.drdy == 0) {
		goto exit; /* spurious interrupt */
	}

	if (lis2dux12->data_ready_handler != NULL) {
		lis2dux12->data_ready_handler(dev, lis2dux12->data_ready_trigger);
	}

exit:
	ret = gpio_pin_interrupt_configure_dt(lis2dux12->drdy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("%s: Not able to configure pin_int", dev->name);
	}
}

static int32_t st_lis2dux12_init_interrupt(const struct device *dev)
{
	const struct lis2dux12_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lis2dux12_pin_int_route_t route;
	int err;

	/* Enable pulsed mode */
	err = lis2dux12_data_ready_mode_set(ctx, LIS2DUX12_DRDY_PULSED);
	if (err < 0) {
		return err;
	}

	/* route data-ready interrupt on int1 */
	err = lis2dux12_pin_int1_route_get(ctx, &route);
	if (err < 0) {
		return err;
	}

	route.drdy = 1;

	err = lis2dux12_pin_int1_route_set(ctx, &route);
	if (err < 0) {
		return err;
	}

	return 0;
}
#endif

const struct lis2dux12_chip_api st_lis2dux12_chip_api = {
	.set_odr_raw = st_lis2dux12_set_odr_raw,
	.set_range = st_lis2dux12_set_range,
	.sample_fetch_accel = st_lis2dux12_sample_fetch_accel,
#ifdef CONFIG_LIS2DUX12_ENABLE_TEMP
	.sample_fetch_temp = st_lis2dux12_sample_fetch_temp,
#endif
#ifdef CONFIG_LIS2DUX12_TRIGGER
	.handle_interrupt = st_lis2dux12_handle_interrupt,
	.init_interrupt = st_lis2dux12_init_interrupt,
#endif
};

int st_lis2dux12_init(const struct device *dev)
{
	const struct lis2dux12_config *const cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id;
	int ret;

	lis2dux12_exit_deep_power_down(ctx);
	k_busy_wait(25000);

	/* check chip ID */
	ret = lis2dux12_device_id_get(ctx, &chip_id);
	if (ret < 0) {
		LOG_ERR("%s: Not able to read dev id", dev->name);
		return ret;
	}

	if (chip_id != LIS2DUX12_ID) {
		LOG_ERR("%s: Invalid chip ID 0x%02x", dev->name, chip_id);
		return -EINVAL;
	}

	/* reset device */
	ret = lis2dux12_init_set(ctx, LIS2DUX12_RESET);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(100);

	LOG_INF("%s: chip id 0x%x", dev->name, chip_id);

	/* Set bdu and if_inc recommended for driver usage */
	lis2dux12_init_set(ctx, LIS2DUX12_SENSOR_ONLY_ON);

	lis2dux12_timestamp_set(ctx, PROPERTY_ENABLE);

#ifdef CONFIG_LIS2DUX12_TRIGGER
	if (cfg->trig_enabled) {
		ret = lis2dux12_trigger_init(dev);
		if (ret < 0) {
			LOG_ERR("%s: Failed to initialize triggers", dev->name);
			return ret;
		}
	}
#endif

	/* set sensor default pm and odr */
	LOG_DBG("%s: pm: %d, odr: %d", dev->name, cfg->pm, cfg->odr);
	ret = st_lis2dux12_set_odr_raw(dev, cfg->odr);
	if (ret < 0) {
		LOG_ERR("%s: odr init error (12.5 Hz)", dev->name);
		return ret;
	}

	/* set sensor default scale (used to convert sample values) */
	LOG_DBG("%s: range is %d", dev->name, cfg->range);
	ret = st_lis2dux12_set_range(dev, cfg->range);
	if (ret < 0) {
		LOG_ERR("%s: range init error %d", dev->name, cfg->range);
		return ret;
	}

	return 0;
}
