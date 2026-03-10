/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_mipid02

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_st_mipid02, CONFIG_VIDEO_LOG_LEVEL);

#define MIPID02_MAX_DATA_LANE_NB	2
struct mipid02_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	const struct device *source_dev;
	struct {
		uint32_t nb_lanes;
		uint8_t lanes[MIPID02_MAX_DATA_LANE_NB];
	} csi;
	uint8_t mode_reg2;
};

struct mipid02_data {
	struct video_format fmt;
	const struct mipid02_format_desc *desc;
	struct video_format_cap *caps;
};

K_HEAP_DEFINE(mipid02_video_caps_heap, CONFIG_VIDEO_ST_MIPID02_CAPS_HEAP_SIZE);

#define MIPID02_REG8(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA8)

#define MIPID02_CLK_LANE_REG1		MIPID02_REG8(0x0002)
#define MIPID02_CLK_LANE_REG1_EN	BIT(0)
#define MIPID02_CLK_LANE_REG1_UI_MSK	0xfc

#define MIPID02_CLK_LANE_REG3		MIPID02_REG8(0x0004)
#define MIPID02_CLK_LANE_REG3_CSI	BIT(1)

#define MIPID02_DATA_LANE0_REG1		MIPID02_REG8(0x0005)
#define MIPID02_DATA_LANE0_REG1_EN	BIT(0)
#define MIPID02_DATA_LANE0_REG1_NORMAL	BIT(1)

#define MIPID02_DATA_LANE0_REG2		MIPID02_REG8(0x0006)
#define MIPID02_DATA_LANE0_REG2_CSI	BIT(0)

#define MIPID02_DATA_LANE1_REG1		MIPID02_REG8(0x0009)
#define MIPID02_DATA_LANE1_REG1_EN	BIT(0)

#define MIPID02_DATA_LANE1_REG2		MIPID02_REG8(0x000a)
#define MIPID02_DATA_LANE1_REG2_CSI	BIT(0)

#define MIPID02_MODE_REG1		MIPID02_REG8(0x0014)
#define MIPID02_MODE_REG1_2LANES	BIT(1)
#define MIPID02_MODE_REG1_SWAP		BIT(2)
#define MIPID02_MODE_REG1_NO_BYPASS	BIT(6)

#define MIPID02_MODE_REG2		MIPID02_REG8(0x0015)
#define MIPID02_MODE_REG2_HSYNC_INV	BIT(1)
#define MIPID02_MODE_REG2_VSYNC_INV	BIT(2)
#define MIPID02_MODE_REG2_PCLK_INV	BIT(3)

#define MIPID02_DATA_ID_REG		MIPID02_REG8(0x0017)

#define MIPID02_DATA_SEL_CTRL_REG	MIPID02_REG8(0x0019)
#define MIPID02_DATA_SEL_CTRL_REG_R_MODE	BIT(2)

#define MIPID02_FMT(fmt, csi_fmt, dt)				\
	{							\
		.pixelformat = VIDEO_PIX_FMT_##fmt,		\
		.csi_pixelformat = VIDEO_PIX_FMT_##csi_fmt,	\
		.csi_datatype = dt,				\
	}

struct mipid02_format_desc {
	uint32_t pixelformat;
	uint32_t csi_pixelformat;
	uint8_t csi_datatype;
};

static const struct mipid02_format_desc mipid02_supported_formats[] = {
	/* Raw 8 bit */
	MIPID02_FMT(SBGGR8, SBGGR8, VIDEO_MIPI_CSI2_DT_RAW8),
	MIPID02_FMT(SGBRG8, SGBRG8, VIDEO_MIPI_CSI2_DT_RAW8),
	MIPID02_FMT(SGRBG8, SGRBG8, VIDEO_MIPI_CSI2_DT_RAW8),
	MIPID02_FMT(SRGGB8, SRGGB8, VIDEO_MIPI_CSI2_DT_RAW8),
	/* Raw 10 bit */
	MIPID02_FMT(SBGGR10, SBGGR10P, VIDEO_MIPI_CSI2_DT_RAW10),
	MIPID02_FMT(SGBRG10, SGBRG10P, VIDEO_MIPI_CSI2_DT_RAW10),
	MIPID02_FMT(SGRBG10, SGRBG10P, VIDEO_MIPI_CSI2_DT_RAW10),
	MIPID02_FMT(SRGGB10, SRGGB10P, VIDEO_MIPI_CSI2_DT_RAW10),
	/* Raw 12 bit */
	MIPID02_FMT(SBGGR12, SBGGR12P, VIDEO_MIPI_CSI2_DT_RAW12),
	MIPID02_FMT(SGBRG12, SGBRG12P, VIDEO_MIPI_CSI2_DT_RAW12),
	MIPID02_FMT(SGRBG12, SGRBG12P, VIDEO_MIPI_CSI2_DT_RAW12),
	MIPID02_FMT(SRGGB12, SRGGB12P, VIDEO_MIPI_CSI2_DT_RAW12),
	/* Yxx */
	MIPID02_FMT(GREY, GREY, VIDEO_MIPI_CSI2_DT_RAW8),
	/* RGB */
	MIPID02_FMT(RGB565, RGB565, VIDEO_MIPI_CSI2_DT_RGB565),
	/* YUV */
	MIPID02_FMT(YUYV, YUYV, VIDEO_MIPI_CSI2_DT_YUV422_8),
	/* JPEG */
	MIPID02_FMT(JPEG, JPEG, 0),
};

static const struct mipid02_format_desc *mipid02_get_format_desc(uint32_t pixelformat)
{
	for (int i = 0; i < ARRAY_SIZE(mipid02_supported_formats); i++) {
		if (pixelformat == mipid02_supported_formats[i].pixelformat ||
		    pixelformat == mipid02_supported_formats[i].csi_pixelformat) {
			return &mipid02_supported_formats[i];
		}
	}

	return NULL;
}

static int mipid02_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct mipid02_data *drv_data = dev->data;
	const struct mipid02_config *cfg = dev->config;
	const struct mipid02_format_desc *desc;
	struct video_format sensor_fmt = *fmt;
	int ret;

	/* Check that format is supported */
	desc = mipid02_get_format_desc(fmt->pixelformat);
	if (!desc) {
		return -EINVAL;
	}

	/* Configure the DT and perform set_fmt on sensor */
	if (desc->csi_datatype != 0) {
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_ID_REG,
					  desc->csi_datatype);
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_SEL_CTRL_REG,
					  MIPID02_DATA_SEL_CTRL_REG_R_MODE);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_SEL_CTRL_REG, 0);
		if (ret < 0) {
			return ret;
		}
	}

	/* Call video_set_format to the sensor */
	sensor_fmt.pixelformat = desc->csi_pixelformat;

	ret = video_set_format(cfg->source_dev, &sensor_fmt);
	if (ret < 0) {
		return ret;
	}

	drv_data->fmt = *fmt;
	drv_data->desc = desc;

	return 0;
}

static int mipid02_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct mipid02_config *cfg = dev->config;
	struct mipid02_data *drv_data = dev->data;
	const struct mipid02_format_desc *desc;
	int ret;

	if (!drv_data->desc) {
		ret = video_get_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}

		desc = mipid02_get_format_desc(fmt->pixelformat);
		if (desc == NULL) {
			LOG_ERR("Sensor format not supported by the ST-MIPID02");
			return -EIO;
		}

		/* Override the pixelformat */
		fmt->pixelformat = desc->pixelformat;

		drv_data->fmt = *fmt;
		drv_data->desc = desc;
	}

	*fmt = drv_data->fmt;

	return 0;
}

static int mipid02_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct mipid02_config *cfg = dev->config;
	struct mipid02_data *drv_data = dev->data;
	int ind = 0, caps_nb = 0;
	int ret;

	ret = video_get_caps(cfg->source_dev, caps);
	if (ret < 0) {
		return ret;
	}

	while (caps->format_caps[ind].pixelformat) {
		if (mipid02_get_format_desc(caps->format_caps[ind].pixelformat)) {
			caps_nb++;
		}
		ind++;
	}

	k_heap_free(&mipid02_video_caps_heap, drv_data->caps);
	drv_data->caps = k_heap_alloc(&mipid02_video_caps_heap,
				      (caps_nb + 1) * sizeof(struct video_format_cap), K_FOREVER);
	if (!drv_data->caps) {
		return -ENOMEM;
	}

	ind = 0;
	for (int i = 0; caps->format_caps[i].pixelformat; i++) {
		const struct mipid02_format_desc *desc =
			mipid02_get_format_desc(caps->format_caps[i].pixelformat);

		if (!desc) {
			continue;
		}

		drv_data->caps[ind] = caps->format_caps[i];
		drv_data->caps[ind].pixelformat = desc->pixelformat;
		ind++;

		if (ind == caps_nb) {
			memset(&drv_data->caps[ind], 0, sizeof(drv_data->caps[ind]));
			break;
		}
	}

	caps->format_caps = drv_data->caps;

	return 0;
}

static int mipid02_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct mipid02_config *cfg = dev->config;
	struct mipid02_data *drv_data = dev->data;
	const struct mipid02_format_desc *desc;
	int32_t link_freq;
	int ret;

	if (!enable) {
		ret = video_stream_stop(cfg->source_dev, type);
		if (ret < 0) {
			return ret;
		}

		/* Disable lanes */
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_CLK_LANE_REG1, 0);
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE0_REG1, 0);
		if (ret < 0) {
			return ret;
		}

		if (cfg->csi.nb_lanes == 2) {
			ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE1_REG1, 0);
			if (ret < 0) {
				return ret;
			}
		}

		return 0;
	}

	desc = mipid02_get_format_desc(drv_data->fmt.pixelformat);
	if (!desc) {
		LOG_ERR("No valid format desc available, should get/set_fmt prior to set_stream");
		return -EIO;
	}

	/* Get link-frequency information from source */
	link_freq = video_get_csi_link_freq(cfg->source_dev,
					    video_bits_per_pixel(desc->csi_pixelformat),
					    cfg->csi.nb_lanes);
	if (link_freq < 0) {
		LOG_ERR("Failed to retrieve source link-frequency");
		return -EIO;
	}

	/* Enable lanes */
	ret = video_write_cci_reg(&cfg->i2c, MIPID02_CLK_LANE_REG1,
				  FIELD_PREP(MIPID02_CLK_LANE_REG1_UI_MSK, 2000000000 / link_freq) |
				  MIPID02_CLK_LANE_REG1_EN);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE0_REG1,
				  MIPID02_DATA_LANE0_REG1_EN | MIPID02_DATA_LANE0_REG1_NORMAL);
	if (ret < 0) {
		return ret;
	}

	if (cfg->csi.nb_lanes == 2) {
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE1_REG1,
					  MIPID02_DATA_LANE1_REG1_EN);
		if (ret < 0) {
			return ret;
		}
	}

	return video_stream_start(cfg->source_dev, type);
}

static int mipid02_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct mipid02_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int mipid02_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct mipid02_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int mipid02_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct mipid02_config *cfg = dev->config;

	return video_enum_frmival(cfg->source_dev, fie);
}

static DEVICE_API(video, mipid02_driver_api) = {
	.set_format = mipid02_set_fmt,
	.get_format = mipid02_get_fmt,
	.get_caps = mipid02_get_caps,
	.set_stream = mipid02_set_stream,
	.set_frmival = mipid02_set_frmival,
	.get_frmival = mipid02_get_frmival,
	.enum_frmival = mipid02_enum_frmival,
};

static int mipid02_init(const struct device *dev)
{
	const struct mipid02_config *cfg = dev->config;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->reset_gpio.port->name);
		return -ENODEV;
	}

	/* Power up sequence */
	if (cfg->reset_gpio.port) {
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(K_MSEC(10));

	if (cfg->reset_gpio.port) {
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
	}

	/* Set CSI mode */
	ret = video_write_cci_reg(&cfg->i2c, MIPID02_CLK_LANE_REG3, MIPID02_CLK_LANE_REG3_CSI);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE0_REG2, MIPID02_DATA_LANE0_REG2_CSI);
	if (ret < 0) {
		return ret;
	}
	if (cfg->csi.nb_lanes == 2) {
		ret = video_write_cci_reg(&cfg->i2c, MIPID02_DATA_LANE1_REG2,
					  MIPID02_DATA_LANE1_REG2_CSI);
		if (ret < 0) {
			return ret;
		}
	}

	/* Configure parallel port synchro signals */
	ret = video_write_cci_reg(&cfg->i2c, MIPID02_MODE_REG2, cfg->mode_reg2);
	if (ret < 0) {
		return ret;
	}

	/* Perform the lanes configuration */
	return video_write_cci_reg(&cfg->i2c, MIPID02_MODE_REG1,
				   MIPID02_MODE_REG1_NO_BYPASS |
				   (cfg->csi.nb_lanes == 2 ? MIPID02_MODE_REG1_2LANES : 0) |
				   (cfg->csi.lanes[0] == 1 ? 0 : MIPID02_MODE_REG1_SWAP));
}

#define SOURCE_DEV(n) DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(n, 0, 0)))

#define MIPID02_INIT(n)										\
	static struct mipid02_data mipid02_data_##n;						\
												\
	static const struct mipid02_config mipid02_cfg_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		.source_dev = SOURCE_DEV(n),							\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.csi.nb_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(n, 0, 0), data_lanes),	\
		.csi.lanes[0] = DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(n, 0, 0),		\
					       data_lanes, 0),					\
		.csi.lanes[1] = COND_CODE_1(DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(n, 0, 0),	\
							data_lanes),				\
					    (0),						\
					    (DT_PROP_BY_IDX(DT_INST_ENDPOINT_BY_ID(n, 0, 0),	\
							    data_lanes, 1))),			\
		.mode_reg2 = (DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 2, 0), hsync_active, 0) ?	\
					MIPID02_MODE_REG2_HSYNC_INV : 0) |			\
			     (DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 2, 0), vsync_active, 0) ?	\
					MIPID02_MODE_REG2_VSYNC_INV : 0) |			\
			     (DT_PROP_OR(DT_INST_ENDPOINT_BY_ID(n, 2, 0), pclk_sample, 0) ?	\
					MIPID02_MODE_REG2_PCLK_INV : 0),			\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &mipid02_init, NULL, &mipid02_data_##n, &mipid02_cfg_##n,	\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &mipid02_driver_api);	\
												\
	VIDEO_DEVICE_DEFINE(mipid02_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(MIPID02_INIT)
