/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT imx_mipi_csi

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/mipi_dphy.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(imx_mipi_csi, CONFIG_VIDEO_LOG_LEVEL);

#define CSI_CFG(dev)  ((const struct csi_config *)((dev)->config))
#define CSI_DATA(dev) ((struct csi_data *)((dev)->data))

#define MIPI_CSI2RX_VERSION			0x00
#define MIPI_CSI2RX_N_LANES			0x04
#define MIPI_CSI2RX_HOST_RESETN			0x08
#define MIPI_CSI2RX_DPHY_STOPSTATE		0x4C
#define MIPI_CSI2RX_N_LANES_MASK		GENMASK(2, 0)
#define MIPI_CSI2RX_HOST_RESETN_ENABLE		BIT(0)
#define MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE	BIT(16)

#define CSI_RESET_DELAY_US			10
#define CSI_STOPSTATE_TIMEOUT_US		50000
#define CSI_STOPSTATE_POLL_DELAY_US		50
#define CSI_DEFAULT_FPS				30

struct csi_pix_format {
	uint32_t pixelformat;
	uint8_t data_type;
	uint8_t bpp;
};

struct csi_config {
	mem_addr_t base;
	const struct device *phy_dev;
	const struct device *sensor_dev;
	uint8_t num_lanes;
};

struct csi_data {
	mem_addr_t regs;
	const struct device *phy;
	const struct device *sensor;

	struct k_mutex lock;

	uint8_t num_lanes;

	struct csi_pix_format csi_fmt;
	struct video_format fmt;
	struct video_frmival frmival;

	bool streaming;
	bool format_set;
	bool frmival_set;
};

/* Supported pixel formats */
static const struct csi_pix_format csi_formats[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_YUYV,
		.data_type = 0x1E,
		.bpp = 16,
	}, {
		.pixelformat = VIDEO_PIX_FMT_RGB24,
		.data_type = 0x24,
		.bpp = 24,
	}, {
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.data_type = 0x22,
		.bpp = 16,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SBGGR8,
		.data_type = 0x2A,
		.bpp = 8,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGBRG8,
		.data_type = 0x2A,
		.bpp = 8,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGRBG8,
		.data_type = 0x2A,
		.bpp = 8,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SRGGB8,
		.data_type = 0x2A,
		.bpp = 8,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SBGGR10,
		.data_type = 0x2B,  /* RAW10 */
		.bpp = 10,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGBRG10,
		.data_type = 0x2B,
		.bpp = 10,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGRBG10,
		.data_type = 0x2B,
		.bpp = 10,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SRGGB10,
		.data_type = 0x2B,
		.bpp = 10,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SBGGR12,
		.data_type = 0x2C,  /* RAW12 */
		.bpp = 12,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGBRG12,
		.data_type = 0x2C,
		.bpp = 12,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SGRBG12,
		.data_type = 0x2C,
		.bpp = 12,
	}, {
		.pixelformat = VIDEO_PIX_FMT_SRGGB12,
		.data_type = 0x2C,
		.bpp = 12,
	},
};

/* Register access helpers */
static inline void csi_write(struct csi_data *csi, uint32_t reg, uint32_t val)
{
	sys_write32(val, csi->regs + reg);
}

static inline uint32_t csi_read(struct csi_data *csi, uint32_t reg)
{
	return sys_read32(csi->regs + reg);
}

/* Format lookup helper */
static const struct csi_pix_format *csi_find_format(uint32_t pixelformat)
{
	for (size_t i = 0; i < ARRAY_SIZE(csi_formats); i++) {
		if (pixelformat == csi_formats[i].pixelformat) {
			return &csi_formats[i];
		}
	}
	return NULL;
}

static int64_t csi_calc_link_freq(struct csi_data *csi)
{
	uint64_t pixel_rate;
	uint64_t link_freq;
	uint32_t fps;

	if (!csi->format_set) {
		LOG_ERR("Format not configured");
		return -EINVAL;
	}

	/* Calculate FPS from frame interval */
	if (csi->frmival_set && csi->frmival.numerator != 0) {
		fps = csi->frmival.denominator / csi->frmival.numerator;
		LOG_DBG("Using configured frame interval: %u/%u (%u fps)",
			csi->frmival.numerator, csi->frmival.denominator, fps);
	} else {
		/* Use default if not configured */
		fps = CSI_DEFAULT_FPS;
		LOG_WRN("Frame interval not set, using default %u fps", fps);
	}

	/* Calculate pixel rate: width * height * fps */
	pixel_rate = (uint64_t)csi->fmt.width * csi->fmt.height * fps;

	/* Calculate link frequency: pixel_rate * bpp / (2 * lanes) */
	link_freq = (pixel_rate * csi->csi_fmt.bpp) / (2 * csi->num_lanes);

	LOG_DBG("Link freq: %llu Hz (pixel_rate=%llu, fps=%u, bpp=%u, lanes=%u)",
		link_freq, pixel_rate, fps, csi->csi_fmt.bpp, csi->num_lanes);

	return (int64_t)link_freq;
}

static int csi_configure_phy(struct csi_data *csi)
{
	struct phy_configure_opts_mipi_dphy opts;
	int64_t link_freq;
	int ret;

	link_freq = csi_calc_link_freq(csi);
	if (link_freq < 0) {
		LOG_ERR("Unable to calculate link frequency: %lld", link_freq);
		return (int)link_freq;
	}

	memset(&opts, 0, sizeof(opts));

	opts.hs_clk_rate = (uint64_t)link_freq * 2;

	/* Check for overflow */
	if (opts.hs_clk_rate < (uint64_t)link_freq) {
		LOG_ERR("hs_clk_rate overflow");
		return -ERANGE;
	}

	opts.lanes = csi->num_lanes;

	LOG_INF("Configuring DPHY: lanes=%u, hs_clk_rate=%llu Hz",
		opts.lanes, opts.hs_clk_rate);

	ret = phy_configure(csi->phy, &opts);
	if (ret) {
		LOG_ERR("Failed to configure PHY: %d", ret);
		return ret;
	}

	return 0;
}

static void csi_host_reset(struct csi_data *csi)
{
	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, 0);
	k_usleep(CSI_RESET_DELAY_US);

	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, MIPI_CSI2RX_HOST_RESETN_ENABLE);
}

static int csi_hw_init(struct csi_data *csi)
{
	uint32_t phy_stopstate_mask;
	uint32_t val;
	int ret;
	uint32_t timeout_us;

	ret = csi_configure_phy(csi);
	if (ret) {
		return ret;
	}

	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, 0);
	k_usleep(CSI_RESET_DELAY_US);

	ret = phy_init(csi->phy);
	if (ret) {
		LOG_ERR("Failed to init PHY: %d", ret);
		goto err_reset;
	}

	ret = phy_power_on(csi->phy);
	if (ret) {
		LOG_ERR("Failed to power on PHY: %d", ret);
		goto err_phy_exit;
	}

	val = FIELD_PREP(MIPI_CSI2RX_N_LANES_MASK, csi->num_lanes - 1);
	csi_write(csi, MIPI_CSI2RX_N_LANES, val);

	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, MIPI_CSI2RX_HOST_RESETN_ENABLE);

	phy_stopstate_mask = MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE |
			     GENMASK(csi->num_lanes - 1, 0);

	timeout_us = CSI_STOPSTATE_TIMEOUT_US;
	while (timeout_us > 0) {
		val = csi_read(csi, MIPI_CSI2RX_DPHY_STOPSTATE);
		if ((val & phy_stopstate_mask) == phy_stopstate_mask) {
			LOG_DBG("PHY lanes in stop state: 0x%08x", val);
			return 0;
		}
		k_usleep(CSI_STOPSTATE_POLL_DELAY_US);
		timeout_us -= CSI_STOPSTATE_POLL_DELAY_US;
	}

	LOG_ERR("Timeout waiting for lanes stop state (got 0x%08x, expected 0x%08x)",
			val, phy_stopstate_mask);

	/* Cleanup on error */
	phy_power_off(csi->phy);
err_phy_exit:
	phy_exit(csi->phy);
err_reset:
	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, 0);
	return -ETIMEDOUT;
}

static void csi_hw_cleanup(struct csi_data *csi)
{
	phy_power_off(csi->phy);
	phy_exit(csi->phy);

	csi_write(csi, MIPI_CSI2RX_HOST_RESETN, 0);
}

static int csi_start_stream(struct csi_data *csi)
{
	int ret;

	if (!csi->format_set) {
		LOG_ERR("Format must be set before starting stream");
		return -EINVAL;
	}

	ret = csi_hw_init(csi);
	if (ret) {
		LOG_ERR("Failed to initialize CSI-2 hardware: %d", ret);
		return ret;
	}

	/* Start sensor streaming if available */
	if (csi->sensor) {
		ret = video_stream_start(csi->sensor, VIDEO_BUF_TYPE_OUTPUT);
		if (ret) {
			LOG_ERR("Failed to start sensor stream: %d", ret);
			csi_hw_cleanup(csi);
			return ret;
		}
		LOG_DBG("Sensor streaming started");
	}

	LOG_INF("CSI-2 stream started");
	return 0;
}

static int csi_stop_stream(struct csi_data *csi)
{
	if (csi->sensor) {
		video_stream_stop(csi->sensor, VIDEO_BUF_TYPE_OUTPUT);
		LOG_DBG("Sensor streaming stopped");
	}

	/* Cleanup hardware */
	csi_hw_cleanup(csi);

	LOG_INF("CSI-2 stream stopped");
	return 0;
}

static int csi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct csi_data *csi = CSI_DATA(dev);
	const struct csi_pix_format *csi_fmt;
	int ret;

	if (fmt == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&csi->lock, K_FOREVER);

	if (csi->streaming) {
		LOG_ERR("Cannot change format while streaming");
		ret = -EBUSY;
		goto unlock;
	}

	csi_fmt = csi_find_format(fmt->pixelformat);
	if (!csi_fmt) {
		LOG_ERR("Unsupported pixel format: 0x%08x", fmt->pixelformat);
		ret = -ENOTSUP;
		goto unlock;
	}

	if (csi->sensor) {
		ret = video_set_format(csi->sensor, fmt);
		if (ret) {
			LOG_ERR("Failed to set sensor format: %d", ret);
			goto unlock;
		}
		LOG_DBG("Sensor format configured");
	}

	csi->csi_fmt = *csi_fmt;
	csi->fmt = *fmt;
	csi->format_set = true;

unlock:
	k_mutex_unlock(&csi->lock);
	return ret;
}

static int csi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct csi_data *csi = CSI_DATA(dev);
	int ret;

	if (fmt == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&csi->lock, K_FOREVER);

	if (!csi->format_set) {
		if (csi->sensor) {
			k_mutex_unlock(&csi->lock);
			return video_get_format(csi->sensor, fmt);
		}

		LOG_WRN("Format not configured");
		ret = -ENODATA;
		goto unlock;
	}

	*fmt = csi->fmt;

unlock:
	k_mutex_unlock(&csi->lock);
	return ret;
}

static int csi_set_stream(const struct device *dev, bool enable,
			      enum video_buf_type type)
{
	struct csi_data *csi = CSI_DATA(dev);
	int ret = 0;

	ARG_UNUSED(type);

	k_mutex_lock(&csi->lock, K_FOREVER);

	if (enable) {
		if (csi->streaming) {
			LOG_DBG("Stream already started");
			goto unlock;
		}

		ret = csi_start_stream(csi);
		if (ret) {
			LOG_ERR("Failed to start stream: %d", ret);
			goto unlock;
		}

		csi->streaming = true;
	} else {
		if (!csi->streaming) {
			LOG_DBG("Stream already stopped");
			goto unlock;
		}

		ret = csi_stop_stream(csi);
		if (ret) {
			LOG_ERR("Failed to stop stream: %d", ret);
			goto unlock;
		}

		csi->streaming = false;
	}

unlock:
	k_mutex_unlock(&csi->lock);
	return ret;
}

static int csi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct csi_data *csi = CSI_DATA(dev);

	if (caps == NULL) {
		return -EINVAL;
	}

	/* Forward to sensor if available */
	if (csi->sensor) {
		return video_get_caps(csi->sensor, caps);
	}

	/* Return basic capabilities */
	memset(caps, 0, sizeof(*caps));
	caps->format_caps = NULL;
	caps->min_vbuf_count = 1;

	return 0;
}

static int csi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct csi_data *csi = CSI_DATA(dev);
	int ret;

	if (frmival == NULL) {
		return -EINVAL;
	}

	if (frmival->numerator == 0 || frmival->denominator == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&csi->lock, K_FOREVER);

	if (csi->streaming) {
		LOG_ERR("Cannot change frame interval while streaming");
		ret = -EBUSY;
		goto unlock;
	}

	if (csi->sensor) {
		ret = video_set_frmival(csi->sensor, frmival);
		if (ret) {
			LOG_ERR("Failed to set sensor frame interval: %d", ret);
			goto unlock;
		}
		LOG_DBG("Sensor frame interval configured");
	}

	csi->frmival = *frmival;
	csi->frmival_set = true;

	LOG_INF("Frame interval set: %u/%u (%u fps)",
		frmival->numerator, frmival->denominator,
		frmival->denominator / frmival->numerator);

unlock:
	k_mutex_unlock(&csi->lock);
	return ret;
}

static int csi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct csi_data *csi = CSI_DATA(dev);
	int ret = 0;

	if (frmival == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&csi->lock, K_FOREVER);

	if (!csi->frmival_set) {
		/* Try to get from sensor if available */
		if (csi->sensor) {
			k_mutex_unlock(&csi->lock);
			return video_get_frmival(csi->sensor, frmival);
		}

		LOG_WRN("Frame interval not configured");
		ret = -ENODATA;
		goto unlock;
	}

	*frmival = csi->frmival;

unlock:
	k_mutex_unlock(&csi->lock);
	return ret;
}

static int csi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	struct csi_data *csi = CSI_DATA(dev);
	int ret;

	if (fie == NULL) {
		return -EINVAL;
	}

	/* Forward enumeration to sensor if available */
	if (csi->sensor) {
		ret = video_enum_frmival(csi->sensor, fie);
		if (ret < 0) {
			return ret;
		}
	}

	/* Auto-configure first enumerated discrete frame interval as default */
	if (fie->index == 0 && fie->type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
		k_mutex_lock(&csi->lock, K_FOREVER);
		if (!csi->frmival_set) {
			csi->frmival = fie->discrete;
			csi->frmival_set = true;
			LOG_DBG("Auto-set frame interval from enumeration: %u/%u",
				fie->discrete.numerator, fie->discrete.denominator);
		}
		k_mutex_unlock(&csi->lock);
	}

	return 0;
}

static DEVICE_API(video, csi_driver_api) = {
	.set_format = csi_set_fmt,
	.get_format = csi_get_fmt,
	.set_stream = csi_set_stream,
	.get_caps = csi_get_caps,
	.set_frmival = csi_set_frmival,
	.get_frmival = csi_get_frmival,
	.enum_frmival = csi_enum_frmival,
};

static int csi_init(const struct device *dev)
{
	struct csi_data *csi = CSI_DATA(dev);
	const struct csi_config *config = CSI_CFG(dev);
	uint32_t version;

	/* Initialize register base address */
	csi->regs = config->base;

	/* Verify PHY device is ready */
	csi->phy = config->phy_dev;
	if (!device_is_ready(csi->phy)) {
		LOG_ERR("PHY device not ready");
		return -ENODEV;
	}
	LOG_DBG("PHY device ready: %s", csi->phy->name);

	/* Initialize optional sensor device reference */
	csi->sensor = config->sensor_dev;
	if (csi->sensor != NULL) {
		if (!device_is_ready(csi->sensor)) {
			LOG_WRN("Sensor device not ready: %s", csi->sensor->name);
			csi->sensor = NULL;
		} else {
			LOG_DBG("Sensor device ready: %s", csi->sensor->name);
		}
	}

	/* Validate number of lanes */
	if (config->num_lanes < 1 || config->num_lanes > 4) {
		LOG_ERR("Invalid number of lanes: %u (must be 1-4)", config->num_lanes);
		return -EINVAL;
	}
	csi->num_lanes = config->num_lanes;

	/* Initialize mutex */
	k_mutex_init(&csi->lock);

	/* Initialize state flags */
	csi->streaming = false;
	csi->format_set = false;
	csi->frmival_set = false;

	/* Reset hardware to known state */
	csi_host_reset(csi);

	/* Read and log version register */
	version = csi_read(csi, MIPI_CSI2RX_VERSION);
	LOG_INF("DWC MIPI CSI-2 version: 0x%08x, %u lanes", version, csi->num_lanes);

	return 0;
}

#define CSI_SENSOR_DEV(inst)					\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, sensor),		\
		    (DEVICE_DT_GET(DT_INST_PHANDLE(inst, sensor))),	\
		    (NULL))						\

#define CSI_DEVICE_INIT(inst)					\
	static struct csi_data csi_data_##inst = {		\
		.streaming = false,					\
		.format_set = false,					\
		.frmival_set = false,					\
	};								\
									\
	static const struct csi_config csi_config_##inst = {	\
		.base = DT_INST_REG_ADDR(inst),				\
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phys)),	\
		.sensor_dev = CSI_SENSOR_DEV(inst),			\
		.num_lanes = DT_INST_PROP(inst, data_lanes),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      csi_init,				\
			      NULL,					\
			      &csi_data_##inst,			\
			      &csi_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_VIDEO_IMX_MIPI_CSI_INIT_PRIORITY,	\
			      &csi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CSI_DEVICE_INIT)
