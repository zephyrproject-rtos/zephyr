/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_mipi_csi

#include <zephyr/devicetree/port-endpoint.h>
#include <zephyr/logging/log.h>

#include "video_device.h"
#include "nxp_imx_mipi_csi_priv.h"

LOG_MODULE_REGISTER(nxp_imx_mipi_csi, CONFIG_VIDEO_LOG_LEVEL);

const struct csi_pix_format csi_formats[] = {
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .data_type = 0x1E, .bpp = 16},
	{.pixelformat = VIDEO_PIX_FMT_RGB24, .data_type = 0x24, .bpp = 24},
	{.pixelformat = VIDEO_PIX_FMT_RGB565, .data_type = 0x22, .bpp = 16},
	{.pixelformat = VIDEO_PIX_FMT_SBGGR8, .data_type = 0x2A, .bpp = 8},
	{.pixelformat = VIDEO_PIX_FMT_SGBRG8, .data_type = 0x2A, .bpp = 8},
	{.pixelformat = VIDEO_PIX_FMT_SGRBG8, .data_type = 0x2A, .bpp = 8},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB8, .data_type = 0x2A, .bpp = 8},
	{.pixelformat = VIDEO_PIX_FMT_SBGGR10, .data_type = 0x2B, .bpp = 10},
	{.pixelformat = VIDEO_PIX_FMT_SGBRG10, .data_type = 0x2B, .bpp = 10},
	{.pixelformat = VIDEO_PIX_FMT_SGRBG10, .data_type = 0x2B, .bpp = 10},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB10, .data_type = 0x2B, .bpp = 10},
	{.pixelformat = VIDEO_PIX_FMT_SBGGR12, .data_type = 0x2C, .bpp = 12},
	{.pixelformat = VIDEO_PIX_FMT_SGBRG12, .data_type = 0x2C, .bpp = 12},
	{.pixelformat = VIDEO_PIX_FMT_SGRBG12, .data_type = 0x2C, .bpp = 12},
	{.pixelformat = VIDEO_PIX_FMT_SRGGB12, .data_type = 0x2C, .bpp = 12},
};

const struct csi_pix_format *nxp_imx_csi_find_format(uint32_t pixelformat)
{
	for (size_t i = 0; i < ARRAY_SIZE(csi_formats); i++) {
		if (pixelformat == csi_formats[i].pixelformat) {
			return &csi_formats[i];
		}
	}

	return NULL;
}

static int nxp_imx_csi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	const struct csi_pix_format *csi_fmt;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->streaming) {
		ret = -EBUSY;
		goto out;
	}

	csi_fmt = nxp_imx_csi_find_format(fmt->pixelformat);
	if (!csi_fmt) {
		ret = -ENOTSUP;
		goto out;
	}

	ret = video_set_format(cfg->sensor_dev, fmt);
	if (ret) {
		goto out;
	}

	data->fmt = *fmt;
	data->csi_fmt = *csi_fmt;
	data->format_set = true;

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int nxp_imx_csi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	int ret;

	ret = video_get_format(cfg->sensor_dev, fmt);
	if (ret) {
		return ret;
	}

	return 0;
}

void nxp_imx_csis_host_reset(struct nxp_imx_mipi_csi_data *data)
{
	sys_write32(0, data->csis_regs + MIPI_CSI2RX_HOST_RESETN);
	k_usleep(CSI_RESET_DELAY_US);
	sys_write32(MIPI_CSI2RX_HOST_RESETN_ENABLE, data->csis_regs + MIPI_CSI2RX_HOST_RESETN);
}

int nxp_imx_csis_wait_stopstate(struct nxp_imx_mipi_csi_data *data, uint8_t lanes)
{
	uint32_t mask = MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE | GENMASK(lanes - 1, 0);
	uint32_t timeout_us = CSI_STOPSTATE_TIMEOUT_US;

	while (timeout_us > 0) {
		uint32_t val = sys_read32(data->csis_regs + MIPI_CSI2RX_DPHY_STOPSTATE);

		if ((val & mask) == mask) {
			LOG_DBG("DPHY lanes in stop state (0x%08x)", val);
			return 0;
		}

		k_usleep(CSI_STOPSTATE_POLL_DELAY_US);
		timeout_us -= CSI_STOPSTATE_POLL_DELAY_US;
	}

	return -ETIMEDOUT;
}

static int nxp_imx_csi_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret = 0;

	ARG_UNUSED(type);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (enable) {
		if (data->streaming) {
			goto out;
		}

		ret = nxp_imx_dphy_enable(data, cfg);
		if (ret) {
			goto out;
		}

		nxp_imx_csis_host_reset(data);

		ret = nxp_imx_csis_wait_stopstate(data, cfg->num_lanes);
		if (ret) {
			(void)nxp_imx_dphy_disable(data, cfg);
			goto out;
		}

		ret = video_stream_start(cfg->sensor_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret) {
			(void)nxp_imx_dphy_disable(data, cfg);
			goto out;
		}

		data->streaming = true;
	} else {
		if (!data->streaming) {
			goto out;
		}

		(void)video_stream_stop(cfg->sensor_dev, VIDEO_BUF_TYPE_OUTPUT);
		(void)nxp_imx_dphy_disable(data, cfg);

		data->streaming = false;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int nxp_imx_csi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	if (caps == NULL) {
		return -EINVAL;
	}

	return video_get_caps(cfg->sensor_dev, caps);
}

static int nxp_imx_csi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret = 0;

	ret = video_set_frmival(cfg->sensor_dev, frmival);
	if (ret) {
		LOG_ERR("Cannot set sensor_dev frmival");
		return ret;
	}

	return ret;
}

static int nxp_imx_csi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret;

	ret = video_get_frmival(cfg->sensor_dev, frmival);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev frmival");
		return ret;
	}

	return 0;
}

static int nxp_imx_csi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret;

	ret = video_enum_frmival(cfg->sensor_dev, fie);
	if (ret) {
		/* -EINVAL / -ENOTSUP are normal end-of-enumeration conditions. */
		if (ret != -EINVAL && ret != -ENOTSUP) {
			LOG_ERR("Cannot enum sensor_dev frmival: %d", ret);
		}
		return ret;
	}

	return 0;
}

static DEVICE_API(video, nxp_imx_mipi_csi_api) = {
	.set_format = nxp_imx_csi_set_fmt,
	.get_format = nxp_imx_csi_get_fmt,
	.set_stream = nxp_imx_csi_set_stream,
	.get_caps = nxp_imx_csi_get_caps,
	.set_frmival = nxp_imx_csi_set_frmival,
	.get_frmival = nxp_imx_csi_get_frmival,
	.enum_frmival = nxp_imx_csi_enum_frmival,
};

static int nxp_imx_mipi_csi_init(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *d = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	d->csis_regs = cfg->csis_base;
	d->dphy_regs = cfg->dphy_base;

	k_mutex_init(&d->lock);

	if (!device_is_ready(cfg->sensor_dev)) {
		return -ENODEV;
	}
	if (cfg->dphy_drv_data == NULL) {
		LOG_ERR("No supported DPHY backend for this SoC");
		return -ENODEV;
	}

	nxp_imx_csis_host_reset(d);
	LOG_INF("i.MX CSI-2 version: 0x%08x, lanes=%u",
		sys_read32(d->csis_regs + MIPI_CSI2RX_VERSION), cfg->num_lanes);

	return 0;
}

#define CSI_SENSOR_DEV(inst)                                                                       \
	DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define DPHY_NODE(inst) DT_INST_PHANDLE(inst, phys)

#define NXP_IMX_DPHY_DRV_DATA(node_id)                                                             \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, nxp_imx95_dphy_rx), \
			(&nxp_imx95_dphy_drv_data), (NULL))

#define NXP_IMX_MIPI_CSI_INIT(inst)                                                                \
	static struct nxp_imx_mipi_csi_data nxp_imx_mipi_csi_data_##inst;                          \
	static const struct nxp_imx_mipi_csi_config nxp_imx_mipi_csi_config_##inst = {             \
		.csis_base = DT_INST_REG_ADDR(inst),                                               \
		.dphy_base = DT_REG_ADDR_BY_IDX(DPHY_NODE(inst), 0),                               \
		.dphy_clock_dev = COND_CODE_1(DT_NODE_HAS_PROP(DPHY_NODE(inst), clocks),	\
			(DEVICE_DT_GET(DT_CLOCKS_CTLR(DPHY_NODE(inst)))), (NULL)),                  \
			 .dphy_clock_subsys = COND_CODE_1(DT_NODE_HAS_PROP(DPHY_NODE(inst), clocks),	\
			((clock_control_subsys_t)DT_CLOCKS_CELL					\
			 (DPHY_NODE(inst), name)), (NULL)), .sensor_dev = CSI_SENSOR_DEV(inst),                     \
				  .num_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0),     \
							   data_lanes),                            \
				  .dphy_drv_data = NXP_IMX_DPHY_DRV_DATA(DPHY_NODE(inst)),         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, nxp_imx_mipi_csi_init, NULL, &nxp_imx_mipi_csi_data_##inst,    \
			      &nxp_imx_mipi_csi_config_##inst, POST_KERNEL,                        \
			      CONFIG_VIDEO_IMX_MIPI_CSI_INIT_PRIORITY, &nxp_imx_mipi_csi_api);     \
	VIDEO_DEVICE_DEFINE(nxp_imx_mipi_csi_##inst, DEVICE_DT_INST_GET(inst),                     \
			    CSI_SENSOR_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(NXP_IMX_MIPI_CSI_INIT)
