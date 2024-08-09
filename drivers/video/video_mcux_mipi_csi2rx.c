/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_csi2rx

#include <fsl_mipi_csi2rx.h>
#include <soc.h>

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_csi);

#define RX_HS_SETTLE_MIN           1
/* CSI-2 Rx maximum lane rate is 1.5 Gbps */
#define MAX_LANE_RATE              1500000000

struct mipi_csi2rx_config {
	const MIPI_CSI2RX_Type *base;
	const struct device *sensor_dev;
};

struct mipi_csi2rx_data {
	csi2rx_config_t csi2rxConfig;
};

static int mipi_csi2rx_cal_rx_hs_settle(uint64_t lane_rate, uint8_t *tHsSettle_EscClk)
{
	uint64_t t_hs_settle_min, t_hs_settle_max, t_hs_settle, rx_hs_settle_cal, t_esc_clk;
	uint32_t esc_clk;

	esc_clk = CLOCK_GetRootClockFreq(kCLOCK_Root_Csi2_Esc);
	if (esc_clk == 0) {
		LOG_ERR("Invalid esc clk value");
		return -EINVAL;
	}

	t_esc_clk = NSEC_PER_SEC / esc_clk;

	/*
	 * From MIPI CSI2 D-PHY specification, the T_HS_SETTLE period should be
	 * in the range of 85ns+6*UI to 145ns+10*UI. The Unit Interval (UI) is
	 * the period for transmitting one bit for each lane. Since the data is
	 * transmitted in DDR mode, UI is defined as:
	 *
	 * UI = 1/2 * period of one bit = 1/2 * 1/link_freq
	 *
	 * Here, we choose a T_HS_SETTLE value which is in the middle of the range.
	 * Thus, tHsSettle_EscClk is calculated using the following formula:
	 * csi2rxConfig.tHsSettle_EscClk = T_HS_SETTLE / period of RxClkInEsc
	 */
	t_hs_settle_min = 85 + 6 * NSEC_PER_SEC / lane_rate;
	t_hs_settle_max = 145 + 10 * NSEC_PER_SEC / lane_rate;
	t_hs_settle = (t_hs_settle_min + t_hs_settle_max) / 2;
	rx_hs_settle_cal = t_hs_settle / t_esc_clk;
	if (!IN_RANGE(rx_hs_settle_cal, RX_HS_SETTLE_MIN, UINT8_MAX)) {
		LOG_ERR("rx_hs_settle calculated value is not correct %llu", rx_hs_settle_cal);
		return -EINVAL;
	}

	*tHsSettle_EscClk = (uint8_t)rx_hs_settle_cal;

	return 0;
}

static int mipi_csi2rx_update_settings(const struct device *dev, enum video_endpoint_id ep)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	uint8_t bpp;
	uint64_t sensor_pixel_rate, sensor_lane_rate, sensor_byte_clk;
	int ret;
	struct video_format fmt;

	ret = video_get_format(config->sensor_dev, ep, &fmt);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev pixel format");
		return ret;
	}

	ret = video_get_ctrl(config->sensor_dev, VIDEO_CID_PIXEL_RATE,
			     (uint64_t *)&sensor_pixel_rate);
	if (ret) {
		LOG_ERR("Can not get sensor_dev pixel rate");
		return ret;
	}

	bpp = video_pix_fmt_bpp(fmt.pixelformat) * 8;
	sensor_lane_rate = sensor_pixel_rate * bpp / drv_data->csi2rxConfig.laneNum;
	if (sensor_lane_rate > MAX_LANE_RATE) {
		LOG_ERR("Rate per lane is too high (max limit: 1.5 Gbps each lane)");
		return -EINVAL;
	}

	sensor_byte_clk = sensor_pixel_rate * bpp / drv_data->csi2rxConfig.laneNum / 8;
	if (sensor_byte_clk >= CLOCK_GetRootClockFreq(kCLOCK_Root_Csi2)) {
		mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2, sensor_byte_clk);
	}

	if (sensor_pixel_rate >= CLOCK_GetRootClockFreq(kCLOCK_Root_Csi2_Ui)) {
		mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2_Ui, sensor_pixel_rate);
	}

	ret = mipi_csi2rx_cal_rx_hs_settle(sensor_lane_rate,
					   &drv_data->csi2rxConfig.tHsSettle_EscClk);

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

	if (fmt == NULL || ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	if (video_get_format(config->sensor_dev, ep, fmt)) {
		return -EIO;
	}

	return 0;
}

static int mipi_csi2rx_stream_start(const struct device *dev)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;

	CSI2RX_Init((MIPI_CSI2RX_Type *)config->base, &drv_data->csi2rxConfig);

	if (video_stream_start(config->sensor_dev)) {
		return -EIO;
	}

	return 0;
}

static int mipi_csi2rx_stream_stop(const struct device *dev)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (video_stream_stop(config->sensor_dev)) {
		return -EIO;
	}

	CSI2RX_Deinit((MIPI_CSI2RX_Type *)config->base);

	return 0;
}

static int mipi_csi2rx_get_caps(const struct device *dev, enum video_endpoint_id ep,
				struct video_caps *caps)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (ep != VIDEO_EP_OUT) {
		return -EINVAL;
	}

	/* Just forward to sensor dev for now */
	return video_get_caps(config->sensor_dev, ep, caps);
}

static inline int mipi_csi2rx_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct mipi_csi2rx_config *config = dev->config;

	if (config->sensor_dev) {
		return video_set_ctrl(config->sensor_dev, cid, value);
	}

	return -ENOTSUP;
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
	return fmt->height * fmt->width * video_pix_fmt_bpp(fmt->pixelformat) * 8;
}

static uint64_t mipi_csi2rx_estimate_lane_rate(const struct video_frmival *cur_fmival,
					       const struct video_frmival *fie_frmival,
					       const struct video_format *cur_format,
					       const struct video_format *fie_format,
					       uint64_t cur_pixel_rate, uint8_t laneNum)
{
	uint64_t est_pixel_rate =
		(mipi_csi2rx_cal_frame_size(cur_format) * fie_frmival->denominator *
		 cur_fmival->numerator * cur_pixel_rate) /
		(mipi_csi2rx_cal_frame_size(fie_format) * fie_frmival->numerator *
		 cur_fmival->denominator);

	return est_pixel_rate * video_pix_fmt_bpp(fie_format->pixelformat) * 8 / laneNum;
}

static int mipi_csi2rx_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
				    struct video_frmival_enum *fie)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	int ret;
	uint64_t cur_pixel_rate, est_lane_rate;
	struct video_frmival cur_frmival;
	struct video_format cur_fmt;
	struct video_format fie_fmt;

	ret = video_enum_frmival(config->sensor_dev, ep, fie);
	if (ret) {
		return ret;
	}

	fie_fmt.height = fie->height;
	fie_fmt.width = fie->width;
	fie_fmt.pixelformat = fie->pixelformat;

	ret = video_get_ctrl(config->sensor_dev, VIDEO_CID_PIXEL_RATE, (uint64_t *)&cur_pixel_rate);
	if (ret) {
		LOG_ERR("Cannot get sensor_dev pixel rate");
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

	if (fie->type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
		est_lane_rate = mipi_csi2rx_estimate_lane_rate(&cur_frmival, &fie->discrete,
							       &cur_fmt, &fie_fmt, cur_pixel_rate,
							       drv_data->csi2rxConfig.laneNum);
		if (est_lane_rate > MAX_LANE_RATE) {
			return -EINVAL;
		}

	} else {
		/* Check the lane rate of the lower bound framerate */
		est_lane_rate = mipi_csi2rx_estimate_lane_rate(&cur_frmival, &fie->stepwise.min,
							       &cur_fmt, &fie_fmt, cur_pixel_rate,
							       drv_data->csi2rxConfig.laneNum);
		if (est_lane_rate > MAX_LANE_RATE) {
			return -EINVAL;
		}

		/* Check the lane rate of the upper bound framerate */
		est_lane_rate = mipi_csi2rx_estimate_lane_rate(&cur_frmival, &fie->stepwise.max,
							       &cur_fmt, &fie_fmt, cur_pixel_rate,
							       drv_data->csi2rxConfig.laneNum);
		if (est_lane_rate > MAX_LANE_RATE) {
			uint64_t max_pixel_rate = MAX_LANE_RATE * drv_data->csi2rxConfig.laneNum /
						  (video_pix_fmt_bpp(fie->pixelformat) * 8);
			fie->stepwise.max.denominator = (mipi_csi2rx_cal_frame_size(&cur_fmt) *
							 max_pixel_rate * cur_frmival.denominator) /
							(mipi_csi2rx_cal_frame_size(&fie_fmt) *
							 cur_pixel_rate * cur_frmival.numerator);
			fie->stepwise.max.numerator = 1;
		}
	}

	return 0;
}

static const struct video_driver_api mipi_csi2rx_driver_api = {
	.get_caps = mipi_csi2rx_get_caps,
	.get_format = mipi_csi2rx_get_fmt,
	.set_format = mipi_csi2rx_set_fmt,
	.stream_start = mipi_csi2rx_stream_start,
	.stream_stop = mipi_csi2rx_stream_stop,
	.set_ctrl = mipi_csi2rx_set_ctrl,
	.set_frmival = mipi_csi2rx_set_frmival,
	.get_frmival = mipi_csi2rx_get_frmival,
	.enum_frmival = mipi_csi2rx_enum_frmival,
};

static int mipi_csi2rx_init(const struct device *dev)
{
	const struct mipi_csi2rx_config *config = dev->config;

	/* Check if there is any sensor device */
	if (!device_is_ready(config->sensor_dev)) {
		return -ENODEV;
	}

	/*
	 * CSI2 escape clock should be in the range [60, 80] Mhz. We set it
	 * to 60 Mhz.
	 */
	mipi_csi2rx_clock_set_freq(kCLOCK_Root_Csi2_Esc, MHZ(60));

	return mipi_csi2rx_update_settings(dev, VIDEO_EP_ANY);
}

#define MIPI_CSI2RX_INIT(n)                                                                        \
	static struct mipi_csi2rx_data mipi_csi2rx_data_##n = {                                    \
		.csi2rxConfig.laneNum = DT_INST_PROP_LEN_OR(n, data_lanes, 2),                     \
	};                                                                                         \
                                                                                                   \
	static const struct mipi_csi2rx_config mipi_csi2rx_config_##n = {                          \
		.base = (MIPI_CSI2RX_Type *)DT_INST_REG_ADDR(n),                                   \
		.sensor_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, sensor)),                           \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &mipi_csi2rx_init, NULL, &mipi_csi2rx_data_##n,                   \
			      &mipi_csi2rx_config_##n, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,    \
			      &mipi_csi2rx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_CSI2RX_INIT)
