/*
 * Copyright (c) 2022-2023 Circuit Valley
 * Copyright (c) 2024-2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sony_imx219

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(imx219, CONFIG_VIDEO_LOG_LEVEL);

#define IMX219_FULL_WIDTH		3280
#define IMX219_FULL_HEIGHT		2464
#define IMX219_CHIP_ID			0x0219

#define IMX219_REG8(addr)		((addr) | VIDEO_REG_ADDR16_DATA8)
#define IMX219_REG16(addr)		((addr) | VIDEO_REG_ADDR16_DATA16_BE)

#define IMX219_CCI_CHIP_ID		IMX219_REG16(0x0000)
#define IMX219_CCI_SOFTWARE_RESET	IMX219_REG8(0x0103)
#define IMX219_CCI_MODE_SELECT		IMX219_REG8(0x0100)
#define IMX219_MODE_SELECT_STANDBY	0x00
#define IMX219_MODE_SELECT_STREAMING	0x01
#define IMX219_CCI_ANALOG_GAIN		IMX219_REG8(0x0157)
#define IMX219_ANALOG_GAIN_DEFAULT	240
#define IMX219_CCI_DIGITAL_GAIN		IMX219_REG16(0x0158)
#define IMX219_DIGITAL_GAIN_DEFAULT	256
#define IMX219_CCI_INTEGRATION_TIME	IMX219_REG16(0x015A)
#define IMX219_INTEGRATION_TIME_DEFAULT	100
#define IMX219_CCI_TESTPATTERN		IMX219_REG16(0x0600)
#define IMX219_CCI_TP_WINDOW_WIDTH	IMX219_REG16(0x0624)
#define IMX219_CCI_TP_WINDOW_HEIGHT	IMX219_REG16(0x0626)
#define IMX219_CCI_CSI_LANE_MODE	IMX219_REG8(0x0114)
#define IMX219_CCI_DPHY_CTRL		IMX219_REG8(0x0128)
#define IMX219_CCI_EXCK_FREQ		IMX219_REG16(0x012a)
#define IMX219_CCI_PREPLLCK_VT_DIV	IMX219_REG8(0x0304)
#define IMX219_CCI_PREPLLCK_OP_DIV	IMX219_REG8(0x0305)
#define IMX219_CCI_OPPXCK_DIV		IMX219_REG8(0x0309)
#define IMX219_CCI_VTPXCK_DIV		IMX219_REG8(0x0301)
#define IMX219_CCI_VTSYCK_DIV		IMX219_REG8(0x0303)
#define IMX219_CCI_OPSYCK_DIV		IMX219_REG8(0x030b)
#define IMX219_CCI_PLL_VT_MPY		IMX219_REG16(0x0306)
#define IMX219_CCI_PLL_OP_MPY		IMX219_REG16(0x030c)
#define IMX219_CCI_LINE_LENGTH_A	IMX219_REG16(0x0162)
#define IMX219_REG_CSI_DATA_FORMAT_A0	0x018c
#define IMX219_REG_CSI_DATA_FORMAT_A1	0x018d
#define IMX219_CCI_BINNING_MODE_H	IMX219_REG8(0x0174)
#define IMX219_CCI_BINNING_MODE_V	IMX219_REG8(0x0175)
#define IMX219_CCI_ORIENTATION		IMX219_REG8(0x0172)
#define IMX219_CCI_FRM_LENGTH_A		IMX219_REG16(0x0160)
#define IMX219_CCI_X_ADD_STA_A		IMX219_REG16(0x0164)
#define IMX219_CCI_X_ADD_END_A		IMX219_REG16(0x0166)
#define IMX219_CCI_Y_ADD_STA_A		IMX219_REG16(0x0168)
#define IMX219_CCI_Y_ADD_END_A		IMX219_REG16(0x016a)
#define IMX219_CCI_X_OUTPUT_SIZE	IMX219_REG16(0x016c)
#define IMX219_CCI_Y_OUTPUT_SIZE	IMX219_REG16(0x016e)
#define IMX219_CCI_X_ODD_INC_A		IMX219_REG8(0x0170)
#define IMX219_CCI_Y_ODD_INC_A		IMX219_REG8(0x0171)
#define IMX219_CCI_DT_PEDESTAL		IMX219_REG16(0xD1EA)
#define IMX219_DT_PEDESTAL_DEFAULT	40

#define IMX219_2DL_LINK_FREQ	456000000
const int64_t imx219_2dl_link_frequency[] = {
	IMX219_2DL_LINK_FREQ,
};

struct imx219_ctrls {
	struct video_ctrl exposure;
	struct video_ctrl brightness;
	struct video_ctrl analogue_gain;
	struct video_ctrl digital_gain;
	struct video_ctrl linkfreq;
	struct video_ctrl test_pattern;
};

struct imx219_data {
	struct imx219_ctrls ctrls;
	struct video_format fmt;
	int fps;
};

struct imx219_config {
	struct i2c_dt_spec i2c;
	uint32_t input_clk_hz;
};

/* Undocumented registers */
static const struct video_reg16 imx219_vendor_regs[] = {
	/* Enable access to registers from 0x3000 to 0x5fff */
	{0x30eb, 0x05},
	{0x30eb, 0x0c},
	{0x300a, 0xff},
	{0x300b, 0xff},
	{0x30eb, 0x05},
	{0x30eb, 0x09},

	/* Extra undocumented registers */
	{0x455e, 0x00},
	{0x471e, 0x4b},
	{0x4767, 0x0f},
	{0x4750, 0x14},
	{0x4540, 0x00},
	{0x47b4, 0x14},
	{0x4713, 0x30},
	{0x478b, 0x10},
	{0x478f, 0x10},
	{0x4793, 0x10},
	{0x4797, 0x0e},
	{0x479b, 0x0e},
};

/* Registers to crop down a resolution to a centered width and height */
static const struct video_reg imx219_init_regs[] = {
	/* MIPI configuration registers */
	{IMX219_CCI_CSI_LANE_MODE, 0x01},	/* 2 Lanes */
	{IMX219_CCI_DPHY_CTRL, 0x00},		/* Timing auto */

	/* Timing and format registers */
	{IMX219_CCI_LINE_LENGTH_A, 3448},
	{IMX219_CCI_X_ODD_INC_A, 1},
	{IMX219_CCI_Y_ODD_INC_A, 1},

	/* Custom defaults */
	{IMX219_CCI_BINNING_MODE_H, 0x00},	/* No binning */
	{IMX219_CCI_BINNING_MODE_V, 0x00}	/* No binning */,
	{IMX219_CCI_DIGITAL_GAIN, IMX219_DIGITAL_GAIN_DEFAULT},
	{IMX219_CCI_ANALOG_GAIN, IMX219_ANALOG_GAIN_DEFAULT},
	{IMX219_CCI_INTEGRATION_TIME, IMX219_INTEGRATION_TIME_DEFAULT},
	{IMX219_CCI_ORIENTATION, 0x03},
};

static const struct video_reg16 imx219_fmt_raw8_regs[] = {
	{IMX219_REG_CSI_DATA_FORMAT_A0, 8},
	{IMX219_REG_CSI_DATA_FORMAT_A1, 8},
};

static const struct video_reg16 imx219_fmt_raw10_regs[] = {
	{IMX219_REG_CSI_DATA_FORMAT_A0, 10},
	{IMX219_REG_CSI_DATA_FORMAT_A1, 10},
};

/* TODO the FPS registers are currently tuned for 1920x1080 cropped resolution */

static const struct video_reg imx219_fps_30_regs[] = {
	{IMX219_CCI_PREPLLCK_VT_DIV, 0x03},	/* Auto */
	{IMX219_CCI_PREPLLCK_OP_DIV, 0x03},	/* Auto */
	{IMX219_CCI_VTPXCK_DIV, 4},		/* Video Timing clock multiplier */
	{IMX219_CCI_VTSYCK_DIV, 1},
	{IMX219_CCI_OPPXCK_DIV, 10},		/* Output pixel clock divider */
	{IMX219_CCI_OPSYCK_DIV, 1},
	{IMX219_CCI_PLL_VT_MPY, 30},		/* Video Timing clock multiplier */
	{IMX219_CCI_PLL_OP_MPY, 50},		/* Output clock multiplier */
};

static const struct video_reg imx219_fps_15_regs[] = {
	{IMX219_CCI_PREPLLCK_VT_DIV, 0x03},	/* Auto */
	{IMX219_CCI_PREPLLCK_OP_DIV, 0x03},	/* Auto */
	{IMX219_CCI_VTPXCK_DIV, 4},		/* Video Timing clock multiplier */
	{IMX219_CCI_VTSYCK_DIV, 1},
	{IMX219_CCI_OPPXCK_DIV, 10},		/* Output pixel clock divider */
	{IMX219_CCI_OPSYCK_DIV, 1},
	{IMX219_CCI_PLL_VT_MPY, 15},		/* Video Timing clock multiplier */
	{IMX219_CCI_PLL_OP_MPY, 50},		/* Output clock multiplier */
};

enum {
	IMX219_RAW8_FULL_FRAME,
	IMX219_RAW10_FULL_FRAME,
};

/* TODO switch to video_set_selection() API for cropping instead */
static const struct video_format_cap imx219_fmts[] = {
	[IMX219_RAW8_FULL_FRAME] = {
		.pixelformat = VIDEO_PIX_FMT_SBGGR8,
		.width_min = 4, .width_max = IMX219_FULL_WIDTH, .width_step = 4,
		.height_min = 4, .height_max = IMX219_FULL_HEIGHT, .height_step = 4,
	},
	[IMX219_RAW10_FULL_FRAME] = {
		.pixelformat = VIDEO_PIX_FMT_SBGGR10P,
		.width_min = 4, .width_max = IMX219_FULL_WIDTH, .width_step = 4,
		.height_min = 4, .height_max = IMX219_FULL_HEIGHT, .height_step = 4,
	},
	{0},
};

static int imx219_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct imx219_config *cfg = dev->config;
	struct imx219_data *drv_data = dev->data;
	struct video_reg crop_regs[] = {
		/* Image crop size */
		{IMX219_CCI_X_ADD_STA_A, (IMX219_FULL_WIDTH - fmt->width) / 2},
		{IMX219_CCI_X_ADD_END_A, (IMX219_FULL_WIDTH + fmt->width) / 2 - 1},
		{IMX219_CCI_Y_ADD_STA_A, (IMX219_FULL_HEIGHT - fmt->height) / 2},
		{IMX219_CCI_Y_ADD_END_A, (IMX219_FULL_HEIGHT + fmt->height) / 2 - 1},

		/* Update the output resolution */
		{IMX219_CCI_X_OUTPUT_SIZE, fmt->width},
		{IMX219_CCI_Y_OUTPUT_SIZE, fmt->height},

		/* Make sure the mipi line is long enough for the new output size */
		{IMX219_CCI_FRM_LENGTH_A, fmt->height + 20},

		/* Test pattern size */
		{IMX219_CCI_TP_WINDOW_WIDTH, fmt->width},
		{IMX219_CCI_TP_WINDOW_HEIGHT, fmt->height}
	};
	size_t idx;
	int ret;

	ret = video_format_caps_index(imx219_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format requested '%s' %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return -ENOTSUP;
	}

	if (fmt->width != imx219_fmts[idx].width_min &&
	    (fmt->width - imx219_fmts[idx].width_min) % imx219_fmts[idx].width_step != 0) {
		LOG_ERR("Unsupported width %u requested", fmt->width);
		return -EINVAL;
	}

	if (fmt->height != imx219_fmts[idx].height_min &&
	    (fmt->height - imx219_fmts[idx].height_min) % imx219_fmts[idx].height_step != 0) {
		LOG_ERR("Unsupported height %u requested", fmt->height);
		return -EINVAL;
	}

	/* Select the base resolution and imaging mode */
	switch (idx) {
	case IMX219_RAW8_FULL_FRAME:
		ret = video_write_cci_multiregs16(&cfg->i2c, imx219_fmt_raw8_regs,
						  ARRAY_SIZE(imx219_fmt_raw8_regs));
		break;
	case IMX219_RAW10_FULL_FRAME:
		ret = video_write_cci_multiregs16(&cfg->i2c, imx219_fmt_raw10_regs,
						  ARRAY_SIZE(imx219_fmt_raw10_regs));
		break;
	default:
		CODE_UNREACHABLE;
		return -EINVAL;
	}
	if (ret < 0) {
		return ret;
	}

	/* Apply a crop window on top of it */
	ret = video_write_cci_multiregs(&cfg->i2c, crop_regs, ARRAY_SIZE(crop_regs));
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
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

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

	return ret;
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

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

	return video_write_cci_reg(&cfg->i2c, IMX219_CCI_MODE_SELECT,
				   on ? IMX219_MODE_SELECT_STREAMING : IMX219_MODE_SELECT_STANDBY);
}

static int imx219_set_ctrl(const struct device *dev, unsigned int cid)
{
	const struct imx219_config *cfg = dev->config;
	struct imx219_data *drv_data = dev->data;
	struct imx219_ctrls *ctrls = &drv_data->ctrls;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		/* Values for normal frame rate, different range for low frame rate mode */
		return video_write_cci_reg(&cfg->i2c, IMX219_CCI_INTEGRATION_TIME,
					   ctrls->exposure.val);
	case VIDEO_CID_ANALOGUE_GAIN:
		return video_write_cci_reg(&cfg->i2c, IMX219_CCI_ANALOG_GAIN,
					   ctrls->analogue_gain.val);
	case VIDEO_CID_GAIN:
		return video_write_cci_reg(&cfg->i2c, IMX219_CCI_DIGITAL_GAIN,
					   ctrls->digital_gain.val);
	case VIDEO_CID_BRIGHTNESS:
		return video_write_cci_reg(&cfg->i2c, IMX219_CCI_DT_PEDESTAL,
					   ctrls->brightness.val);
	case VIDEO_CID_TEST_PATTERN:
		return video_write_cci_reg(&cfg->i2c, IMX219_CCI_TESTPATTERN,
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
		(struct video_ctrl_range){.min = 0x0000, .max = 0xffff, .step = 1,
					  .def = IMX219_INTEGRATION_TIME_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->brightness, dev, VIDEO_CID_BRIGHTNESS,
		(struct video_ctrl_range){.min = 0x00, .max = 0x3ff, .step = 1,
					  .def = IMX219_DT_PEDESTAL_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->analogue_gain, dev, VIDEO_CID_ANALOGUE_GAIN,
		(struct video_ctrl_range){.min = 0x00, .max = 0xff, .step = 1,
					  .def = IMX219_ANALOG_GAIN_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->digital_gain, dev, VIDEO_CID_DIGITAL_GAIN,
		(struct video_ctrl_range){.min = 0x000, .max = 0xfff, .step = 1,
					  .def = IMX219_DIGITAL_GAIN_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->linkfreq, dev, VIDEO_CID_LINK_FREQ,
				       0, imx219_2dl_link_frequency,
				       ARRAY_SIZE(imx219_2dl_link_frequency));
	if (ret < 0) {
		return ret;
	}

	ctrls->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				    imx219_test_pattern_menu);
}

static int imx219_set_input_clk(const struct device *dev, uint32_t rate_hz)
{
	const struct imx219_config *cfg = dev->config;

	if (rate_hz < MHZ(6) || rate_hz > MHZ(27) || rate_hz % MHZ(1) != 0) {
		LOG_ERR("Unsupported INCK freq (%d Hz)\n", rate_hz);
		return -EINVAL;
	}

	return video_write_cci_reg(&cfg->i2c, IMX219_CCI_EXCK_FREQ, (rate_hz / MHZ(1)) << 8);
}

static int imx219_init(const struct device *dev)
{
	const struct imx219_config *cfg = dev->config;
	struct video_format fmt = {
		.width = imx219_fmts[0].width_min,
		.height = imx219_fmts[0].height_min,
		.pixelformat = imx219_fmts[0].pixelformat,
	};
	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = 15,
	};
	uint32_t reg;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	k_sleep(K_MSEC(1));

	ret = video_write_cci_reg(&cfg->i2c, IMX219_CCI_SOFTWARE_RESET, 1);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(6)); /* t5 */

	ret = video_read_cci_reg(&cfg->i2c, IMX219_CCI_CHIP_ID, &reg);
	if (ret < 0) {
		return ret;
	}

	if (reg != IMX219_CHIP_ID) {
		LOG_ERR("Wrong chip ID 0x%04x instead of 0x%04x", reg, IMX219_CHIP_ID);
		return -ENODEV;
	}

	ret = imx219_set_input_clk(dev, cfg->input_clk_hz);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_multiregs16(&cfg->i2c, imx219_vendor_regs,
					  ARRAY_SIZE(imx219_vendor_regs));
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_multiregs(&cfg->i2c, imx219_init_regs,
					ARRAY_SIZE(imx219_init_regs));
	if (ret < 0) {
		return ret;
	}

	ret = imx219_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	ret = imx219_set_frmival(dev, &frmival);
	if (ret < 0) {
		return ret;
	}

	return imx219_init_ctrls(dev);
}

#define IMX219_INIT(n)                                                                             \
	static struct imx219_data imx219_data_##n;                                                 \
                                                                                                   \
	static const struct imx219_config imx219_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.input_clk_hz = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &imx219_init, NULL, &imx219_data_##n, &imx219_cfg_##n,            \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &imx219_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(imx219_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(IMX219_INIT)
