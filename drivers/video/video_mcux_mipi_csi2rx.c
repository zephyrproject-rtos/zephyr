/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mipi_csi2rx

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <fsl_mipi_csi2rx.h>

LOG_MODULE_REGISTER(video_mipi_csi2rx, CONFIG_VIDEO_LOG_LEVEL);

#define DEFAULT_CAMERA_FRAME_RATE  30

struct mipi_csi2rx_config {
	const MIPI_CSI2RX_Type *base;
	const struct device *sensor_dev;
};

struct mipi_csi2rx_data {
	csi2rx_config_t csi2rxConfig;
};

static int mipi_csi2rx_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			       struct video_format *fmt)
{
	const struct mipi_csi2rx_config *config = dev->config;
	struct mipi_csi2rx_data *drv_data = dev->data;
	csi2rx_config_t csi2rxConfig = {0};
	uint8_t i = 0;

	/*
	 * Initialize the MIPI CSI2
	 *
	 * From D-PHY specification, the T-HSSETTLE should in the range of 85ns+6*UI to 145ns+10*UI
	 * UI is Unit Interval, equal to the duration of any HS state on the Clock Lane
	 *
	 * T-HSSETTLE = csi2rxConfig.tHsSettle_EscClk * (Tperiod of RxClkInEsc)
	 *
	 * csi2rxConfig.tHsSettle_EscClk setting for camera:
	 *
	 *    Resolution  |  frame rate  |  T_HS_SETTLE
	 *  =============================================
	 *     720P       |     30       |     0x12
	 *  ---------------------------------------------
	 *     720P       |     15       |     0x17
	 *  ---------------------------------------------
	 *      VGA       |     30       |     0x1F
	 *  ---------------------------------------------
	 *      VGA       |     15       |     0x24
	 *  ---------------------------------------------
	 *     QVGA       |     30       |     0x1F
	 *  ---------------------------------------------
	 *     QVGA       |     15       |     0x24
	 *  ---------------------------------------------
	 */
	static const uint32_t csi2rxHsSettle[][4] = {
		{
			1280,
			720,
			30,
			0x12,
		},
		{
			1280,
			720,
			15,
			0x17,
		},
		{
			640,
			480,
			30,
			0x1F,
		},
		{
			640,
			480,
			15,
			0x24,
		},
		{
			320,
			240,
			30,
			0x1F,
		},
		{
			320,
			240,
			15,
			0x24,
		},
	};

	for (i = 0; i < ARRAY_SIZE(csi2rxHsSettle); i++) {
		if ((fmt->width == csi2rxHsSettle[i][0]) && (fmt->height == csi2rxHsSettle[i][1]) &&
		    (DEFAULT_CAMERA_FRAME_RATE == csi2rxHsSettle[i][2])) {
			csi2rxConfig.tHsSettle_EscClk = csi2rxHsSettle[i][3];
			break;
		}
	}

	if (i == ARRAY_SIZE(csi2rxHsSettle)) {
		LOG_ERR("Unsupported resolution");
		return -ENOTSUP;
	}

	drv_data->csi2rxConfig = csi2rxConfig;

	if (video_set_format(config->sensor_dev, ep, fmt)) {
		return -EIO;
	}

	return 0;
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

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	/* Just forward to sensor dev for now */
	return video_get_caps(config->sensor_dev, ep, caps);
}

static const struct video_driver_api mipi_csi2rx_driver_api = {
	.get_caps = mipi_csi2rx_get_caps,
	.get_format = mipi_csi2rx_get_fmt,
	.set_format = mipi_csi2rx_set_fmt,
	.stream_start = mipi_csi2rx_stream_start,
	.stream_stop = mipi_csi2rx_stream_stop,
};

static int mipi_csi2rx_init(const struct device *dev)
{
	const struct mipi_csi2rx_config *config = dev->config;

	/* Check if there is any sensor device */
	if (!device_is_ready(config->sensor_dev)) {
		return -ENODEV;
	}

	return 0;
}

#define MIPI_CSI2RX_INIT(n)                                                                        \
	static struct mipi_csi2rx_data mipi_csi2rx_data_##n = {                                    \
		.csi2rxConfig.laneNum =                                                            \
			DT_PROP_LEN(DT_CHILD(DT_CHILD(DT_INST_CHILD(n, ports), port_1), endpoint), \
				    data_lanes),                                                   \
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
