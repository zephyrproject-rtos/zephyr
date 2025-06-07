/*
 * Copyright (c) 2022-2023 Circuit Valley
 * Copyright (c) 2024-2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sony_imx219

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(imx219, CONFIG_VIDEO_LOG_LEVEL);

#define IMX219_FULL_WIDTH		3280
#define IMX219_FULL_HEIGHT		2464

#define IMX219_REG8(addr)		((addr) | VIDEO_REG_ADDR16_DATA8)
#define IMX219_REG16(addr)		((addr) | VIDEO_REG_ADDR16_DATA16_BE)

#define IMX219_REG_CHIP_ID		IMX219_REG16(0x0000)
#define IMX219_REG_SOFTWARE_RESET	IMX219_REG8(0x0103)
#define IMX219_REG_MODE_SELECT		IMX219_REG8(0x0100)
#define IMX219_MODE_SELECT_STANDBY	0x00
#define IMX219_MODE_SELECT_STREAMING	0x00
#define IMX219_REG_ANALOG_GAIN		IMX219_REG8(0x0157)
#define IMX219_REG_DIGITAL_GAIN		IMX219_REG16(0x0158)
#define IMX219_REG_INTEGRATION_TIME	IMX219_REG16(0x015A)
#define IMX219_REG_TESTPATTERN		IMX219_REG16(0x0600)
#define IMX219_REG_TP_WINDOW_WIDTH	IMX219_REG16(0x0624)
#define IMX219_REG_TP_WINDOW_HEIGHT	IMX219_REG16(0x0626)
#define IMX219_REG_MODE_SELECT		IMX219_REG8(0x0100)
#define IMX219_REG_CSI_LANE_MODE	IMX219_REG8(0x0114)
#define IMX219_REG_DPHY_CTRL		IMX219_REG8(0x0128)
#define IMX219_REG_EXCK_FREQ		IMX219_REG16(0x012a)
#define IMX219_REG_PREPLLCK_VT_DIV	IMX219_REG8(0x0304)
#define IMX219_REG_PREPLLCK_OP_DIV	IMX219_REG8(0x0305)
#define IMX219_REG_OPPXCK_DIV		IMX219_REG8(0x0309)
#define IMX219_REG_VTPXCK_DIV		IMX219_REG8(0x0301)
#define IMX219_REG_VTSYCK_DIV		IMX219_REG8(0x0303)
#define IMX219_REG_OPSYCK_DIV		IMX219_REG8(0x030b)
#define IMX219_REG_PLL_VT_MPY		IMX219_REG16(0x0306)
#define IMX219_REG_PLL_OP_MPY		IMX219_REG16(0x030c)
#define IMX219_REG_LINE_LENGTH_A	IMX219_REG16(0x0162)
#define IMX219_REG_CSI_DATA_FORMAT_A0	IMX219_REG8(0x018c)
#define IMX219_REG_CSI_DATA_FORMAT_A1	IMX219_REG8(0x018d)
#define IMX219_REG_BINNING_MODE_H	IMX219_REG8(0x0174)
#define IMX219_REG_BINNING_MODE_V	IMX219_REG8(0x0175)
#define IMX219_REG_ORIENTATION		IMX219_REG8(0x0172)
#define IMX219_REG_FRM_LENGTH_A		IMX219_REG16(0x0160)
#define IMX219_REG_X_ADD_STA_A		IMX219_REG16(0x0164)
#define IMX219_REG_X_ADD_END_A		IMX219_REG16(0x0166)
#define IMX219_REG_Y_ADD_STA_A		IMX219_REG16(0x0168)
#define IMX219_REG_Y_ADD_END_A		IMX219_REG16(0x016a)
#define IMX219_REG_X_OUTPUT_SIZE	IMX219_REG16(0x016c)
#define IMX219_REG_Y_OUTPUT_SIZE	IMX219_REG16(0x016e)
#define IMX219_REG_X_ODD_INC_A		IMX219_REG8(0x0170)
#define IMX219_REG_Y_ODD_INC_A		IMX219_REG8(0x0171)
#define IMX219_REG_DT_PEDESTAL		IMX219_REG16(0xD1EA)

struct imx219_ctrls {
	struct video_ctrl exposure;
	struct video_ctrl brightness;
	struct video_ctrl analogue_gain;
	struct video_ctrl digital_gain;
	struct video_ctrl test_pattern;
};

struct imx219_data {
	struct imx219_ctrls ctrls;
	struct video_format fmt;
	int fps;
};

struct imx219_config {
	struct i2c_dt_spec i2c;
};

/* Registers to crop down a resolution to a centered width and height */
static const struct video_reg imx219_init_regs[] = {
	{IMX219_REG_MODE_SELECT, 0x00},		/* Standby */

	/* Enable access to registers from 0x3000 to 0x5fff */
	{IMX219_REG8(0x30eb), 0x05},
	{IMX219_REG8(0x30eb), 0x0c},
	{IMX219_REG8(0x300a), 0xff},
	{IMX219_REG8(0x300b), 0xff},
	{IMX219_REG8(0x30eb), 0x05},
	{IMX219_REG8(0x30eb), 0x09},

	/* MIPI configuration registers */
	{IMX219_REG_CSI_LANE_MODE, 0x01},	/* 2 Lanes */
	{IMX219_REG_DPHY_CTRL, 0x00},		/* Timing auto */

	/* Clock configuration registers */
	{IMX219_REG_EXCK_FREQ, 24 << 8},	/* 24 MHz */

	/* Undocumented registers */
	{IMX219_REG8(0x455e), 0x00},
	{IMX219_REG8(0x471e), 0x4b},
	{IMX219_REG8(0x4767), 0x0f},
	{IMX219_REG8(0x4750), 0x14},
	{IMX219_REG8(0x4540), 0x00},
	{IMX219_REG8(0x47b4), 0x14},
	{IMX219_REG8(0x4713), 0x30},
	{IMX219_REG8(0x478b), 0x10},
	{IMX219_REG8(0x478f), 0x10},
	{IMX219_REG8(0x4793), 0x10},
	{IMX219_REG8(0x4797), 0x0e},
	{IMX219_REG8(0x479b), 0x0e},

	/* Timing and format registers */
	{IMX219_REG_LINE_LENGTH_A, 3448},
	{IMX219_REG_X_ODD_INC_A, 1},
	{IMX219_REG_Y_ODD_INC_A, 1},

	/* Custom defaults */
	{IMX219_REG_BINNING_MODE_H, 0x00},	/* No binning */
	{IMX219_REG_BINNING_MODE_V, 0x00}	/* No binning */,
	{IMX219_REG_DIGITAL_GAIN, 5000},
	{IMX219_REG_ANALOG_GAIN, 240},
	{IMX219_REG_INTEGRATION_TIME, 500},
	{IMX219_REG_ORIENTATION, 0x03},
};

#define IMX219_REGS_CROP(width, height)                                                            \
	{IMX219_REG_X_ADD_STA_A, (IMX219_FULL_WIDTH - (width)) / 2},                               \
	{IMX219_REG_X_ADD_END_A, (IMX219_FULL_WIDTH + (width)) / 2 - 1},                           \
	{IMX219_REG_Y_ADD_STA_A, (IMX219_FULL_HEIGHT - (height)) / 2},                             \
	{IMX219_REG_Y_ADD_END_A, (IMX219_FULL_HEIGHT + (height)) / 2 - 1}

static const struct video_reg imx219_fmt_raw10_regs[] = {
	{IMX219_REG_CSI_DATA_FORMAT_A0, 10},
	{IMX219_REG_CSI_DATA_FORMAT_A1, 10},
	{IMX219_REG_OPPXCK_DIV, 10},
};

static const struct video_reg imx219_fps_30_regs[] = {
	{IMX219_REG_PREPLLCK_VT_DIV, 0x03},	/* Auto */
	{IMX219_REG_PREPLLCK_OP_DIV, 0x03},	/* Auto */
	{IMX219_REG_VTPXCK_DIV, 4},		/* Video Timing clock multiplier */
	{IMX219_REG_VTSYCK_DIV, 1},
	{IMX219_REG_OPPXCK_DIV, 10},		/* Output pixel clock divider */
	{IMX219_REG_OPSYCK_DIV, 1},
	{IMX219_REG_PLL_VT_MPY, 30},		/* Video Timing clock multiplier */
	{IMX219_REG_PLL_OP_MPY, 50},		/* Output clock multiplier */
};

static const struct video_reg imx219_fps_15_regs[] = {
	{IMX219_REG_PREPLLCK_VT_DIV, 0x03},	/* Auto */
	{IMX219_REG_PREPLLCK_OP_DIV, 0x03},	/* Auto */
	{IMX219_REG_VTPXCK_DIV, 4},		/* Video Timing clock multiplier */
	{IMX219_REG_VTSYCK_DIV, 1},
	{IMX219_REG_OPPXCK_DIV, 10},		/* Output pixel clock divider */
	{IMX219_REG_OPSYCK_DIV, 1},
	{IMX219_REG_PLL_VT_MPY, 15},		/* Video Timing clock multiplier */
	{IMX219_REG_PLL_OP_MPY, 50},		/* Output clock multiplier */
};

static const struct video_reg imx219_size_1920x1080_regs[] = {
	IMX219_REGS_CROP(1920, 1080),
	{IMX219_REG_X_OUTPUT_SIZE, 1920},
	{IMX219_REG_Y_OUTPUT_SIZE, 1080},
	{IMX219_REG_FRM_LENGTH_A, 1080 + 20},
	/* Test pattern size */
	{IMX219_REG_TP_WINDOW_WIDTH, 1920},
	{IMX219_REG_TP_WINDOW_HEIGHT, 1080},
};

static const struct video_format_cap imx219_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_SBGGR8,
		.width_min = 1920, .width_max = 1920, .width_step = 0,
		.height_min = 1080, .height_max = 1080, .height_step = 0,
	},
	{0},
};

static int imx219_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct imx219_config *cfg = dev->config;
	struct imx219_data *drv_data = dev->data;
	size_t idx;
	int ret;

	ret = video_format_caps_index(imx219_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format requested '%s' %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return -ENOTSUP;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, IMX219_MODE_SELECT_STANDBY);
	if (ret < 0) {
		return ret;
	}

	switch (idx) {
	case 0:
		ret = video_write_cci_multiregs(&cfg->i2c, imx219_fmt_raw10_regs,
						ARRAY_SIZE(imx219_fmt_raw10_regs));
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_multiregs(&cfg->i2c, imx219_size_1920x1080_regs,
						ARRAY_SIZE(imx219_size_1920x1080_regs));
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		CODE_UNREACHABLE;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, IMX219_MODE_SELECT_STREAMING);
	if (ret < 0) {
		return ret;
	}

	drv_data->fmt = *fmt;

	return 0;
}

static int imx219_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct imx219_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int imx219_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output capabilities supported");
		return -EINVAL;
	}

	caps->min_line_count = LINE_COUNT_HEIGHT;
	caps->max_line_count = LINE_COUNT_HEIGHT;
	caps->format_caps = imx219_fmts;

	return 0;
}

enum {
	IMX219_FRMIVAL_30FPS,
	IMX219_FRMIVAL_15FPS,
};

static int imx219_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;

	switch (fie->index) {
	case IMX219_FRMIVAL_30FPS:
		fie->discrete.numerator = 1;
		fie->discrete.denominator = 30;
		break;
	case IMX219_FRMIVAL_15FPS:
		fie->discrete.numerator = 1;
		fie->discrete.denominator = 15;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int imx219_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct imx219_config *cfg = dev->config;
	struct imx219_data *drv_data = dev->data;
	struct video_frmival_enum fie = {
		.discrete = *frmival,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.format = &drv_data->fmt,
	};
	int ret;

	video_closest_frmival(dev, &fie);

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, IMX219_MODE_SELECT_STANDBY);
	if (ret < 0) {
		return ret;
	}

	switch (fie.index) {
	case IMX219_FRMIVAL_30FPS:
		ret = video_write_cci_multiregs(&cfg->i2c, imx219_fps_30_regs,
						ARRAY_SIZE(imx219_fps_30_regs));
		frmival->numerator = 1;
		frmival->denominator = drv_data->fps = 30;
		break;
	case IMX219_FRMIVAL_15FPS:
		ret = video_write_cci_multiregs(&cfg->i2c, imx219_fps_15_regs,
						ARRAY_SIZE(imx219_fps_15_regs));
		frmival->numerator = 1;
		frmival->denominator = drv_data->fps = 15;
		break;
	default:
		CODE_UNREACHABLE;
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, IMX219_MODE_SELECT_STREAMING);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int imx219_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct imx219_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = drv_data->fps;

	return 0;
}

static int imx219_set_stream(const struct device *dev, bool on, enum video_buf_type type)
{
	const struct imx219_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, on ? 0x01 : 0x00);
}

static int imx219_set_ctrl(const struct device *dev, unsigned int cid)
{
	const struct imx219_config *cfg = dev->config;
	struct imx219_data *drv_data = dev->data;
	struct imx219_ctrls *ctrls = &drv_data->ctrls;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		/* Values for normal frame rate, different range for low frame rate mode */
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_INTEGRATION_TIME,
					   ctrls->exposure.val);
	case VIDEO_CID_ANALOGUE_GAIN:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_ANALOG_GAIN,
					   ctrls->analogue_gain.val);
	case VIDEO_CID_DIGITAL_GAIN:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_DIGITAL_GAIN,
					   ctrls->digital_gain.val);
	case VIDEO_CID_BRIGHTNESS:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_DT_PEDESTAL,
					   ctrls->brightness.val);
	case VIDEO_CID_TEST_PATTERN:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_TESTPATTERN,
					   ctrls->test_pattern.val);
	default:
		LOG_WRN("Control not supported");
		return -ENOTSUP;
	}
}

static const DEVICE_API(video, imx219_driver_api) = {
	.set_stream = imx219_set_stream,
	.set_ctrl = imx219_set_ctrl,
	.set_format = imx219_set_fmt,
	.get_format = imx219_get_fmt,
	.get_caps = imx219_get_caps,
	.set_frmival = imx219_set_frmival,
	.get_frmival = imx219_get_frmival,
	.enum_frmival = imx219_enum_frmival,
};

static const char *const imx219_test_pattern_menu[] = {
	"Off",
	"Solid color",
	"100% color bars",
	"Fade to grey color bar",
	"PN9",
	"16 split color bar",
	"16 split inverted color bar",
	"Column counter",
	"Inverted column counter",
	"PN31",
	NULL,
};

static int imx219_init_ctrls(const struct device *dev)
{
	struct imx219_data *drv_data = dev->data;
	struct imx219_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_init_ctrl(
		&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
		(struct video_ctrl_range){.min = 0x0000, .max = 0xffff, .step = 1, .def = 1000});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
		(struct video_ctrl_range){.min = 0x00, .max = 0x3ff, .step = 1, .def = 40});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->analogue_gain, dev, VIDEO_CID_ANALOGUE_GAIN,
		(struct video_ctrl_range){.min = 0x00, .max = 0xff, .step = 1, .def = 0x00});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->digital_gain, dev, VIDEO_CID_DIGITAL_GAIN,
		(struct video_ctrl_range){.min = 0x000, .max = 0xfff, .step = 1, .def = 0x100});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				   imx219_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int imx219_init(const struct device *dev)
{
	const struct imx219_config *cfg = dev->config;
	struct video_format fmt;
	struct video_frmival frmival;
	uint32_t reg;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	k_sleep(K_MSEC(1));

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_SOFTWARE_RESET, 1);
	if (ret < 0) {
		goto i2c_err;
	}

	k_sleep(K_MSEC(6)); /* t5 */

	ret = video_read_cci_reg(&cfg->i2c, IMX219_REG_CHIP_ID, &reg);
	if (ret < 0) {
		goto i2c_err;
	}

	if (reg != 0x0219) {
		LOG_ERR("Wrong chip ID %04x", reg);
		return -ENODEV;
	}

	ret = video_write_cci_multiregs(&cfg->i2c, imx219_init_regs, ARRAY_SIZE(imx219_init_regs));
	if (ret < 0) {
		goto i2c_err;
	}

	fmt.width = imx219_fmts[0].width_min;
	fmt.height = imx219_fmts[0].height_min;
	fmt.pixelformat = imx219_fmts[0].pixelformat;

	ret = imx219_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	frmival.numerator = 1;
	frmival.denominator = 15;

	ret = imx219_set_frmival(dev, &frmival);
	if (ret < 0) {
		return ret;
	}

	return imx219_init_ctrls(dev);
i2c_err:
	LOG_ERR("I2C error during %s initialization: %s", dev->name, strerror(-ret));
	return ret;
}

#define IMX219_INIT(n)                                                                             \
	static struct imx219_data imx219_data_##n;                                                 \
                                                                                                   \
	static const struct imx219_config imx219_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &imx219_init, NULL, &imx219_data_##n, &imx219_cfg_##n,            \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &imx219_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(imx219_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(IMX219_INIT)
