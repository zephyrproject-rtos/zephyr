/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT adi_max96717

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_max96717, CONFIG_VIDEO_LOG_LEVEL);

#define MAX96717_POLL_RETRIES  10
#define MAX96717_POLL_DELAY_MS 100

#define MAX96717_REG8(addr) ((addr) | VIDEO_REG_ADDR16_DATA8)

#define MAX96717_REG_REG13        MAX96717_REG8(0x000D)
#define MAX96717_REG_CTRL3        MAX96717_REG8(0x0013)
#define MAX96717_CTRL3_LOCKED     BIT(3)
#define MAX96717_CTRL3_ERROR      BIT(2)
#define MAX96717_CTRL3_CMU_LOCKED BIT(1)

#define MAX96717_LINK_LOCK_RETRIES  50
#define MAX96717_LINK_LOCK_DELAY_MS 20

static const char *max96717_variant_model(uint8_t id)
{
	if (id == 0xBF) {
		return "MAX96717";
	}

	return "unknown";
}

struct max96717_config {
	struct i2c_dt_spec i2c;
	const struct device *source_dev;
	uint8_t csi_rx_num_lanes;
};

struct max96717_data {
	struct video_format fmt;
};

static int max96717_log_device_id(const struct i2c_dt_spec *i2c)
{
	uint32_t dev_id;
	int ret;

	ret = video_read_cci_reg(i2c, MAX96717_REG_REG13, &dev_id);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID (err %d)", ret);
		return ret;
	}

	LOG_DBG("Found %s (ID=0x%02X) on %s@0x%02x", max96717_variant_model(dev_id), dev_id,
		i2c->bus->name, i2c->addr);

	return 0;
}

static int max96717_check_link_lock(const struct i2c_dt_spec *i2c)
{
	uint32_t val;
	int ret;

	for (int i = 0; i < MAX96717_LINK_LOCK_RETRIES; i++) {
		ret = video_read_cci_reg(i2c, MAX96717_REG_CTRL3, &val);
		if (ret < 0) {
			LOG_ERR("Failed to read CTRL3 (err %d)", ret);
			return ret;
		}

		if (val & MAX96717_CTRL3_LOCKED) {
			LOG_INF("GMSL2 link locked (CTRL3=0x%02X)", val);
			break;
		}

		LOG_DBG("Waiting for GMSL2 link lock (%d/%d, CTRL3=0x%02X)", i + 1,
			MAX96717_LINK_LOCK_RETRIES, val);
		k_msleep(MAX96717_LINK_LOCK_DELAY_MS);
	}

	if (!(val & MAX96717_CTRL3_LOCKED)) {
		LOG_WRN("GMSL2 link not locked (CTRL3=0x%02X)", val);
	}

	if (!(val & MAX96717_CTRL3_CMU_LOCKED)) {
		LOG_WRN("CMU not locked");
	}

	if (val & MAX96717_CTRL3_ERROR) {
		LOG_WRN("ERRB asserted — check GMSL link integrity");
	}

	return 0;
}

static int max96717_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev != NULL) {
		return video_get_caps(cfg->source_dev, caps);
	}

	return -ENOTSUP;
}

static int max96717_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev != NULL) {
		return video_get_format(cfg->source_dev, fmt);
	}

	return -ENOTSUP;
}

static int max96717_set_format(const struct device *dev, struct video_format *fmt)
{

	return -ENOSYS;
}

static int max96717_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{

	return -ENOSYS;
}

static int max96717_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_set_frmival(cfg->source_dev, frmival);
}

static int max96717_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_get_frmival(cfg->source_dev, frmival);
}

static int max96717_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_enum_frmival(cfg->source_dev, fie);
}

static DEVICE_API(video, max96717_api) = {
	.get_caps = max96717_get_caps,
	.get_format = max96717_get_format,
	.set_format = max96717_set_format,
	.set_stream = max96717_set_stream,
	.set_frmival = max96717_set_frmival,
	.get_frmival = max96717_get_frmival,
	.enum_frmival = max96717_enum_frmival,
};

static int max96717_init(const struct device *dev)
{
	const struct max96717_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = max96717_log_device_id(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	ret = max96717_check_link_lock(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

/* Resolve upstream source device from CSI input port@0.                       \
 * When no sensor is wired (e.g. PATGEN mode), port@0 does not exist           \
 * and src_dev is NULL — the MAX96717 itself is the frame source.              \
 */
#define SOURCE_DEV(inst)                                                                           \
	COND_CODE_1(                                                                               \
		DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)),                                \
		(DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))),        \
		(NULL))

/* Number of CSI-2 data lanes from port@0 endpoint data-lanes property.        \
 * When port@0 has no endpoint (PATGEN mode), defaults to 0 (unused).          \
 */
#define CSI_RX_NUM_LANES(inst)                                                                     \
	COND_CODE_1(                                                                               \
		DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)),                                \
		(DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), data_lanes)),                    \
		(0))

#define MAX96717_INIT(inst)                                                                        \
	static const struct max96717_config max96717_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.source_dev = SOURCE_DEV(inst),                                                    \
		.csi_rx_num_lanes = CSI_RX_NUM_LANES(inst),                                        \
	};                                                                                         \
	static struct max96717_data max96717_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max96717_init, NULL, &max96717_data_##inst,                    \
			      &max96717_config_##inst, POST_KERNEL,                                \
			      CONFIG_VIDEO_MAX96717_INIT_PRIORITY, &max96717_api);                 \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(max96717_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(MAX96717_INIT)
