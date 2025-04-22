/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_csi2rx

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#include <fsl_mipi_csi2rx.h>

#include "video_device.h"

LOG_MODULE_REGISTER(video_mipi_csi2rx, CONFIG_VIDEO_LOG_LEVEL);

#define MAX_SUPPORTED_PIXEL_RATE MHZ(96)

#define ABS(a, b) (a > b ? a - b : b - a)

struct mipi_csi2rx_config {
	const MIPI_CSI2RX_Type *base;
	const struct device *sensor_dev;
};

struct mipi_csi2rx_data {
	csi2rx_config_t csi2rxConfig;
	const struct device *clock_dev;
	clock_control_subsys_t clock_root;
	clock_control_subsys_t clock_ui;
	clock_control_subsys_t clock_esc;
};

struct mipi_csi2rx_tHsSettleEscClk_config {
	uint64_t pixel_rate;
	uint8_t tHsSettle_EscClk;
};

/* Must be in pixel rate ascending order */
const struct mipi_csi2rx_tHsSettleEscClk_config tHsSettleEscClk_configs[] = {
	{MHZ(24), 0x24},
	{MHZ(48), 0x12},
	{MHZ(96), 0x09},
};

static int mipi_csi2rx_update_settings(const struct device *dev, enum video_endpoint_id ep)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	uint8_t bpp;
	uint32_t root_clk_rate, ui_clk_rate, sensor_byte_clk, best_match;
	int ret, ind = 0;
	struct video_format fmt;
	struct video_control sensor_rate = {.id = VIDEO_CID_PIXEL_RATE, .val64 = -1};

	ret = video_get_format(config->sensor_dev, ep, &fmt);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev pixel format");
		return ret;
	}

	video_get_ctrl(config->sensor_dev, &sensor_rate);
	if (!IN_RANGE(sensor_rate.val64, 0, MAX_SUPPORTED_PIXEL_RATE)) {
		LOG_ERR("Sensor pixel rate is not supported %lld", sensor_rate.val64);
		return -ENOTSUP;
	}

	bpp = video_bits_per_pixel(fmt.pixelformat);
	sensor_byte_clk = sensor_rate.val64 * bpp / drv_data->csi2rxConfig.laneNum / BITS_PER_BYTE;

	ret = clock_control_get_rate(drv_data->clock_dev, drv_data->clock_root, &root_clk_rate);
	if (ret) {
		return ret;
	}

	if (sensor_byte_clk > root_clk_rate) {
		ret = clock_control_set_rate(drv_data->clock_dev, drv_data->clock_root,
					     (clock_control_subsys_rate_t)sensor_byte_clk);
		if (ret) {
			return ret;
		}
	}

	ret = clock_control_get_rate(drv_data->clock_dev, drv_data->clock_ui, &ui_clk_rate);
	if (ret) {
		return ret;
	}

	if (sensor_rate.val64 > ui_clk_rate) {
		ret = clock_control_set_rate(
			drv_data->clock_dev, drv_data->clock_ui,
			(clock_control_subsys_rate_t)(uint32_t)sensor_rate.val64);
		if (ret) {
			return ret;
		}
	}

	/* Find the supported sensor_pixel_rate closest to the desired one */
	best_match = tHsSettleEscClk_configs[ind].pixel_rate;
	for (uint8_t i = 0; i < ARRAY_SIZE(tHsSettleEscClk_configs); i++) {
		if (ABS(tHsSettleEscClk_configs[i].pixel_rate, sensor_rate.val64) <
		    ABS(tHsSettleEscClk_configs[i].pixel_rate, best_match)) {
			best_match = tHsSettleEscClk_configs[i].pixel_rate;
			ind = i;
		}
	}

	drv_data->csi2rxConfig.tHsSettle_EscClk = tHsSettleEscClk_configs[ind].tHsSettle_EscClk;

	return ret;
}

static int mipi_csi2rx_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (video_set_format(config->sensor_dev, ep, fmt)) {
		return -EIO;
	}

	return mipi_csi2rx_update_settings(dev, ep);
}

static int mipi_csi2rx_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (fmt == NULL || (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL)) {
		return -EINVAL;
	}

	if (video_get_format(config->sensor_dev, ep, fmt)) {
		return -EIO;
	}

	return 0;
}

static int mipi_csi2rx_set_stream(const struct device *dev, bool enable)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (enable) {
		struct mipi_csi2rx_data *drv_data = dev->data;

		CSI2RX_Init((MIPI_CSI2RX_Type *)config->base, &drv_data->csi2rxConfig);
		if (video_stream_start(config->sensor_dev)) {
			return -EIO;
		}
	} else {
		if (video_stream_stop(config->sensor_dev)) {
			return -EIO;
		}
		CSI2RX_Deinit((MIPI_CSI2RX_Type *)config->base);
	}

	return 0;
}

static int mipi_csi2rx_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	/* Just forward to sensor dev for now */
	return video_get_caps(config->sensor_dev, ep, caps);
}

static int mipi_csi2rx_set_frmival(const struct device *dev, enum video_endpoint_id ep,
				   struct video_frmival *frmival)
{
	const struct mipi_csi2rx_config *config = dev->config;
	int ret;

	ret = video_set_frmival(config->sensor_dev, ep, frmival);
	if (ret) {
		LOG_ERR("Cannot set sensor_dev frmival");
		return ret;
	}

	ret = mipi_csi2rx_update_settings(dev, ep);

	return ret;
}

static int mipi_csi2rx_get_frmival(const struct device *dev, enum video_endpoint_id ep,
				   struct video_frmival *frmival)
{
	const struct mipi_csi2rx_config *config = dev->config;

	return video_get_frmival(config->sensor_dev, ep, frmival);
}

static uint64_t mipi_csi2rx_cal_frame_size(const struct video_format *fmt)
{
	return fmt->height * fmt->width * video_bits_per_pixel(fmt->pixelformat);
}

static uint64_t mipi_csi2rx_estimate_pixel_rate(const struct video_frmival *cur_fmival,
						const struct video_frmival *fie_frmival,
						const struct video_format *cur_format,
						const struct video_format *fie_format,
						uint64_t cur_pixel_rate, uint8_t laneNum)
{
	return mipi_csi2rx_cal_frame_size(cur_format) * fie_frmival->denominator *
	       cur_fmival->numerator * cur_pixel_rate /
	       (mipi_csi2rx_cal_frame_size(fie_format) * fie_frmival->numerator *
		cur_fmival->denominator);
}

static int mipi_csi2rx_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				    struct video_frmival_enum *fie)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	int ret;
	uint64_t est_pixel_rate;
	struct video_frmival cur_frmival;
	struct video_format cur_fmt;
	struct video_control sensor_rate = {.id = VIDEO_CID_PIXEL_RATE, .val64 = -1};

	ret = video_enum_frmival(config->sensor_dev, ep, fie);
	if (ret) {
		return ret;
	}

	ret = video_get_frmival(config->sensor_dev, ep, &cur_frmival);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev frame rate");
		return ret;
	}

	ret = video_get_format(config->sensor_dev, ep, &cur_fmt);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev format");
		return ret;
	}

	ret = video_get_ctrl(config->sensor_dev, &sensor_rate);
	if (ret) {
		return ret;
	}

	if (fie->type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
		est_pixel_rate = mipi_csi2rx_estimate_pixel_rate(
			&cur_frmival, &fie->discrete, &cur_fmt, fie->format, sensor_rate.val64,
			drv_data->csi2rxConfig.laneNum);
		if (est_pixel_rate > MAX_SUPPORTED_PIXEL_RATE) {
			return -EINVAL;
		}

	} else {
		/* Check the lane rate of the lower bound framerate */
		est_pixel_rate = mipi_csi2rx_estimate_pixel_rate(
			&cur_frmival, &fie->stepwise.min, &cur_fmt, fie->format, sensor_rate.val64,
			drv_data->csi2rxConfig.laneNum);
		if (est_pixel_rate > MAX_SUPPORTED_PIXEL_RATE) {
			return -EINVAL;
		}

		/* Check the lane rate of the upper bound framerate */
		est_pixel_rate = mipi_csi2rx_estimate_pixel_rate(
			&cur_frmival, &fie->stepwise.max, &cur_fmt, fie->format, sensor_rate.val64,
			drv_data->csi2rxConfig.laneNum);
		if (est_pixel_rate > MAX_SUPPORTED_PIXEL_RATE) {
			fie->stepwise.max.denominator =
				(mipi_csi2rx_cal_frame_size(&cur_fmt) * MAX_SUPPORTED_PIXEL_RATE *
				 cur_frmival.denominator) /
				(mipi_csi2rx_cal_frame_size(fie->format) * sensor_rate.val64 *
				 cur_frmival.numerator);
			fie->stepwise.max.numerator = 1;
		}
	}

	return 0;
}

static DEVICE_API(video, mipi_csi2rx_driver_api) = {
	.get_caps = mipi_csi2rx_get_caps,
	.get_format = mipi_csi2rx_get_fmt,
	.set_format = mipi_csi2rx_set_fmt,
	.set_stream = mipi_csi2rx_set_stream,
	.set_frmival = mipi_csi2rx_set_frmival,
	.get_frmival = mipi_csi2rx_get_frmival,
	.enum_frmival = mipi_csi2rx_enum_frmival,
};

static int mipi_csi2rx_init(const struct device *dev)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	int ret;

	/* Check if there is any sensor device */
	if (!device_is_ready(config->sensor_dev)) {
		return -ENODEV;
	}

	/*
	 * CSI2 escape clock should be in the range [60, 80] Mhz. We set it
	 * to 60 Mhz.
	 */
	ret = clock_control_set_rate(drv_data->clock_dev, drv_data->clock_esc,
				     (clock_control_subsys_rate_t)MHZ(60));
	if (ret) {
		return ret;
	}

	return mipi_csi2rx_update_settings(dev, VIDEO_EP_ALL);
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 1, 0)))

#define MIPI_CSI2RX_INIT(n)                                                                        \
	static struct mipi_csi2rx_data mipi_csi2rx_data_##n = {                                    \
		.csi2rxConfig.laneNum = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(n, 1, 0), data_lanes),  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_root = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),      \
		.clock_ui = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 1, name),        \
		.clock_esc = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 2, name),       \
	};                                                                                         \
                                                                                                   \
	static const struct mipi_csi2rx_config mipi_csi2rx_config_##n = {                          \
		.base = (MIPI_CSI2RX_Type *)DT_INST_REG_ADDR(n),                                   \
		.sensor_dev = SOURCE_DEV(n),                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &mipi_csi2rx_init, NULL, &mipi_csi2rx_data_##n,                   \
			      &mipi_csi2rx_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &mipi_csi2rx_driver_api);                                            \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(mipi_csi2rx_##n, DEVICE_DT_INST_GET(n), SOURCE_DEV(n));

DT_INST_FOREACH_STATUS_OKAY(MIPI_CSI2RX_INIT)
