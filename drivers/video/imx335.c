/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sony_imx335

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_imx335, CONFIG_VIDEO_LOG_LEVEL);

#define IMX335_NATIVE_WIDTH	2592
#define IMX335_NATIVE_HEIGHT	1944

#define IMX335_BIN_2X2_WIDTH	1296
#define IMX335_BIN_2X2_HEIGHT	972

#define IMX335_PIXEL_RATE	396000000

enum imx335_res {
	IMX335_RES_2592x1944,
	IMX335_RES_1296x972,
};

enum imx335_framerate {
	IMX335_25_FPS,
	IMX335_30_FPS,
	IMX335_50_FPS,
	IMX335_60_FPS,
};

struct imx335_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
	uint32_t input_clk;
};

struct imx335_ctrls {
	struct video_ctrl gain;
	struct video_ctrl exposure;
	struct video_ctrl pixel_rate;
};

struct imx335_data {
	struct imx335_ctrls ctrls;
	struct video_format fmt;
	uint32_t frame_rate;
	bool enabled;
};

#define IMX335_REG8(addr)  ((addr) | VIDEO_REG_ADDR16_DATA8)
#define IMX335_REG16(addr)  ((addr) | VIDEO_REG_ADDR16_DATA16_LE)
#define IMX335_REG24(addr)  ((addr) | VIDEO_REG_ADDR16_DATA24_LE)

#define IMX335_STANDBY			IMX335_REG8(0x3000)
#define IMX335_STANDBY_OPERATING	0x00
#define IMX335_STANDBY_STANDBY		BIT(0)
#define IMX335_REGHOLD			IMX335_REG8(0x3001)
#define IMX335_XMSTA			IMX335_REG8(0x3002)
#define IMX335_BCWAIT_TIME		IMX335_REG8(0x300c)
#define IMX335_CPWAIT_TIME		IMX335_REG8(0x300d)
#define IMX335_WINMODE			IMX335_REG8(0x3018)
#define IMX335_HTRIMMING_START		IMX335_REG16(0x302c)
#define IMX335_HNUM			IMX335_REG16(0x302e)
#define IMX335_VMAX			IMX335_REG24(0x3030)
#define IMX335_HMAX			IMX335_REG16(0x3034)
#define IMX335_VMAX_DEFAULT		4500
#define IMX335_OPB_SIZE_V		IMX335_REG8(0x304c)
#define IMX335_ADBIT			IMX335_REG8(0x3050)
#define IMX335_Y_OUT_SIZE		IMX335_REG16(0x3056)
#define IMX335_SHR0			IMX335_REG24(0x3058)
#define IMX335_SHR0_MIN			9
#define IMX335_SHR0_MIN_BINNING		17
#define IMX335_AREA2_WIDTH_1		IMX335_REG16(0x3072)
#define IMX335_AREA3_ST_ADR_1		IMX335_REG16(0x3074)
#define IMX335_AREA3_WIDTH_1		IMX335_REG16(0x3076)
#define IMX335_GAIN			IMX335_REG16(0x30e8)
#define IMX335_GAIN_MIN			(0 * 1000)
#define IMX335_GAIN_MAX			(72 * 1000)
#define IMX335_GAIN_UNIT_MDB		300
#define IMX335_INCKSEL1			IMX335_REG16(0x314c)
#define IMX335_INCKSEL2_PLL_IF_GC	IMX335_REG8(0x315a)
#define IMX335_INCKSEL3			IMX335_REG8(0x3168)
#define IMX335_INCKSEL4			IMX335_REG8(0x316a)
#define IMX335_HADD_VADD		IMX335_REG8(0x3199)
#define IMX335_MDBIT			IMX335_REG8(0x319d)
#define IMX335_XVS_XHS_DRV		IMX335_REG8(0x31a1)
#define IMX335_TCYCLE			IMX335_REG8(0x3300)
#define IMX335_ADBIT1			IMX335_REG16(0x341c)
#define IMX335_LANEMODE			IMX335_REG8(0x3a01)
static const struct video_reg imx335_init_params[] = {
	{IMX335_STANDBY, 0x01},
	{IMX335_XMSTA, 0x00},
	{IMX335_HTRIMMING_START, 0x3c},
	{IMX335_HNUM, 0x0a20},
	{IMX335_Y_OUT_SIZE, 0x0798},
	{IMX335_AREA3_ST_ADR_1, 0x00c8},
	{IMX335_AREA3_WIDTH_1, 0x0f30},
	{IMX335_XVS_XHS_DRV, 0x00},
};

static const struct video_reg16 imx335_fixed_regs[] = {
	{0x3288, 0x21},
	{0x328a, 0x02},
	{0x3414, 0x05},
	{0x3416, 0x18},
	{0x3648, 0x01},
	{0x364a, 0x04},
	{0x364c, 0x04},
	{0x3678, 0x01},
	{0x367c, 0x31},
	{0x367e, 0x31},
	{0x3706, 0x10},
	{0x3708, 0x03},
	{0x3714, 0x02},
	{0x3715, 0x02},
	{0x3716, 0x01},
	{0x3717, 0x03},
	{0x371c, 0x3d},
	{0x371d, 0x3f},
	{0x372c, 0x00},
	{0x372d, 0x00},
	{0x372e, 0x46},
	{0x372f, 0x00},
	{0x3730, 0x89},
	{0x3731, 0x00},
	{0x3732, 0x08},
	{0x3733, 0x01},
	{0x3734, 0xfe},
	{0x3735, 0x05},
	{0x3740, 0x02},
	{0x375d, 0x00},
	{0x375e, 0x00},
	{0x375f, 0x11},
	{0x3760, 0x01},
	{0x3768, 0x1b},
	{0x3769, 0x1b},
	{0x376a, 0x1b},
	{0x376b, 0x1b},
	{0x376c, 0x1a},
	{0x376d, 0x17},
	{0x376e, 0x0f},
	{0x3776, 0x00},
	{0x3777, 0x00},
	{0x3778, 0x46},
	{0x3779, 0x00},
	{0x377a, 0x89},
	{0x377b, 0x00},
	{0x377c, 0x08},
	{0x377d, 0x01},
	{0x377e, 0x23},
	{0x377f, 0x02},
	{0x3780, 0xd9},
	{0x3781, 0x03},
	{0x3782, 0xf5},
	{0x3783, 0x06},
	{0x3784, 0xa5},
	{0x3788, 0x0f},
	{0x378a, 0xd9},
	{0x378b, 0x03},
	{0x378c, 0xeb},
	{0x378d, 0x05},
	{0x378e, 0x87},
	{0x378f, 0x06},
	{0x3790, 0xf5},
	{0x3792, 0x43},
	{0x3794, 0x7a},
	{0x3796, 0xa1},
	{0x37b0, 0x36},
	{0x3a00, 0x01},
};

static const struct video_reg imx335_bin_2x2[] = {
	{IMX335_WINMODE, 0x01},
	{IMX335_Y_OUT_SIZE, 0x03D8},
	{IMX335_AREA2_WIDTH_1, 0x0030},
	{IMX335_AREA3_ST_ADR_1, 0x00A8},
	{IMX335_AREA3_WIDTH_1, 0x0F60},
	{IMX335_HADD_VADD, 0x30},
	{IMX335_TCYCLE, 0x01},
	/* Fixed regs for 2/2 binning */
	{IMX335_REG8(0x3078), 0x04},
	{IMX335_REG8(0x3079), 0xFD},
	{IMX335_REG8(0x307A), 0x04},
	{IMX335_REG8(0x307B), 0xFE},
	{IMX335_REG8(0x307C), 0x04},
	{IMX335_REG8(0x307D), 0xFB},
	{IMX335_REG8(0x307E), 0x04},
	{IMX335_REG8(0x307F), 0x02},
	{IMX335_REG8(0x3080), 0x04},
	{IMX335_REG8(0x3081), 0xFD},
	{IMX335_REG8(0x3082), 0x04},
	{IMX335_REG8(0x3083), 0xFE},
	{IMX335_REG8(0x3084), 0x04},
	{IMX335_REG8(0x3085), 0xFB},
	{IMX335_REG8(0x3086), 0x04},
	{IMX335_REG8(0x3087), 0x02},
	{IMX335_REG8(0x30A4), 0x77},
	{IMX335_REG8(0x30A8), 0x20},
	{IMX335_REG8(0x30A9), 0x00},
	{IMX335_REG8(0x30AC), 0x08},
	{IMX335_REG8(0x30AD), 0x08},
	{IMX335_REG8(0x30B0), 0x20},
	{IMX335_REG8(0x30B1), 0x00},
	{IMX335_REG8(0x30B4), 0x10},
	{IMX335_REG8(0x30B5), 0x10},
	{IMX335_REG16(0x30B6), 0x0000},
	{IMX335_REG16(0x3112), 0x0010},
	{IMX335_REG16(0x3116), 0x0010},
};

/* Note that this only contains registers that need to be reset when switching away from
 * 2x2 binning mode
 */
static const struct video_reg imx335_bin_none[] = {
	{IMX335_WINMODE, 0x00},
	{IMX335_Y_OUT_SIZE, 0x07AC},
	{IMX335_AREA2_WIDTH_1, 0x0028},
	{IMX335_AREA3_ST_ADR_1, 0x00B0},
	{IMX335_AREA3_WIDTH_1, 0x0F58},
	{IMX335_HADD_VADD, 0x00},
	{IMX335_TCYCLE, 0x00},
	/* Reset fixed regs from binning to all-pixel scan */
	{IMX335_REG8(0x3078), 0x01},
	{IMX335_REG8(0x3079), 0x02},
	{IMX335_REG8(0x307A), 0xFF},
	{IMX335_REG8(0x307B), 0x02},
	{IMX335_REG8(0x307C), 0x00},
	{IMX335_REG8(0x307D), 0x00},
	{IMX335_REG8(0x307E), 0x00},
	{IMX335_REG8(0x307F), 0x00},
	{IMX335_REG8(0x3080), 0x01},
	{IMX335_REG8(0x3081), 0x02},
	{IMX335_REG8(0x3082), 0xFF},
	{IMX335_REG8(0x3083), 0x02},
	{IMX335_REG8(0x3084), 0x00},
	{IMX335_REG8(0x3085), 0x00},
	{IMX335_REG8(0x3086), 0x00},
	{IMX335_REG8(0x3087), 0x00},
	{IMX335_REG8(0x30A4), 0x33},
	{IMX335_REG8(0x30A8), 0x10},
	{IMX335_REG8(0x30A9), 0x04},
	{IMX335_REG8(0x30AC), 0x00},
	{IMX335_REG8(0x30AD), 0x00},
	{IMX335_REG8(0x30B0), 0x10},
	{IMX335_REG8(0x30B1), 0x08},
	{IMX335_REG8(0x30B4), 0x00},
	{IMX335_REG8(0x30B5), 0x00},
	{IMX335_REG16(0x30B6), 0x0000},
	{IMX335_REG16(0x3112), 0x0008},
	{IMX335_REG16(0x3116), 0x0008},
};

static const struct video_reg imx335_mode_2l_10b[] = {
	{IMX335_ADBIT, 0x00},
	{IMX335_MDBIT, 0x00},
	{IMX335_ADBIT1, 0x01ff},
	{IMX335_LANEMODE, 0x01},
};

static const struct video_reg imx335_inck_74mhz[] = {
	{IMX335_BCWAIT_TIME, 0xb6},
	{IMX335_CPWAIT_TIME, 0x7f},
	{IMX335_INCKSEL1, 0x80},
	{IMX335_INCKSEL2_PLL_IF_GC, 0x03},
	{IMX335_INCKSEL3, 0x68},
	{IMX335_INCKSEL4, 0x7f},
};

static const struct video_reg imx335_inck_27mhz[] = {
	{IMX335_BCWAIT_TIME, 0x42},
	{IMX335_CPWAIT_TIME, 0x2e},
	{IMX335_INCKSEL1, 0xb0},
	{IMX335_INCKSEL2_PLL_IF_GC, 0x02},
	{IMX335_INCKSEL3, 0x8f},
	{IMX335_INCKSEL4, 0x7e},
};

static const struct video_reg imx335_inck_24mhz[] = {
	{IMX335_BCWAIT_TIME, 0x3b},
	{IMX335_CPWAIT_TIME, 0x2a},
	{IMX335_INCKSEL1, 0xc6},
	{IMX335_INCKSEL2_PLL_IF_GC, 0x02},
	{IMX335_INCKSEL3, 0xa0},
	{IMX335_INCKSEL4, 0x7e},
};

static const struct video_reg imx335_inck_18mhz[] = {
	{IMX335_BCWAIT_TIME, 0x2d},
	{IMX335_CPWAIT_TIME, 0x1f},
	{IMX335_INCKSEL1, 0x84},
	{IMX335_INCKSEL2_PLL_IF_GC, 0x01},
	{IMX335_INCKSEL3, 0x6b},
	{IMX335_INCKSEL4, 0x7d},
};

static const struct video_reg imx335_inck_6mhz[] = {
	{IMX335_BCWAIT_TIME, 0x0f},
	{IMX335_CPWAIT_TIME, 0x0b},
	{IMX335_INCKSEL1, 0xc6},
	{IMX335_INCKSEL2_PLL_IF_GC, 0x00},
	{IMX335_INCKSEL3, 0xa0},
	{IMX335_INCKSEL4, 0x7c},
};

static const struct video_format_cap imx335_fmts[] = {
	/* all-pixel scan mode */
	[IMX335_RES_2592x1944] = {
		.pixelformat = VIDEO_PIX_FMT_SRGGB10P,
		.width_min = IMX335_NATIVE_WIDTH,
		.width_max = IMX335_NATIVE_WIDTH,
		.height_min = IMX335_NATIVE_HEIGHT,
		.height_max = IMX335_NATIVE_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	/* 2x2 binning mode */
	[IMX335_RES_1296x972] = {
		.pixelformat = VIDEO_PIX_FMT_SRGGB10P,
		.width_min = IMX335_BIN_2X2_WIDTH,
		.width_max = IMX335_BIN_2X2_WIDTH,
		.height_min = IMX335_BIN_2X2_HEIGHT,
		.height_max = IMX335_BIN_2X2_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	{0}
};

static const uint32_t imx335_framerates[] = {
	[IMX335_25_FPS] = 25,
	[IMX335_30_FPS] = 30,
	[IMX335_50_FPS] = 50,
	[IMX335_60_FPS] = 60,
};

static int imx335_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct imx335_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int imx335_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = imx335_fmts;
	return 0;
}

static int imx335_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct imx335_config *cfg = dev->config;
	struct imx335_data *drv_data = dev->data;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, IMX335_STANDBY,
				  enable ? IMX335_STANDBY_OPERATING : IMX335_STANDBY_STANDBY);
	if (ret < 0) {
		LOG_ERR("Failed to set standby register\n");
		return ret;
	}

	drv_data->enabled = enable;

	k_sleep(K_USEC(20));

	return 0;
}

static int imx335_set_ctrl_gain(const struct device *dev)
{
	const struct imx335_config *cfg = dev->config;
	struct imx335_data *drv_data = dev->data;
	struct imx335_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	/* Apply gain upon conversion to gain unit */
	ret = video_write_cci_reg(&cfg->i2c, IMX335_GAIN, ctrls->gain.val / IMX335_GAIN_UNIT_MDB);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 0);
}

static int imx335_set_ctrl_exposure(const struct device *dev)
{
	const struct imx335_config *cfg = dev->config;
	struct imx335_data *drv_data = dev->data;
	struct imx335_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	/* Since we never update VMAX, we can use the default value directly */
	ret = video_write_cci_reg(&cfg->i2c, IMX335_SHR0,
				  IMX335_VMAX_DEFAULT - ctrls->exposure.val);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 0);
}

static int imx335_set_ctrl(const struct device *dev, unsigned int cid)
{
	switch (cid) {
	case VIDEO_CID_ANALOGUE_GAIN:
		return imx335_set_ctrl_gain(dev);
	case VIDEO_CID_EXPOSURE:
		return imx335_set_ctrl_exposure(dev);
	default:
		return -ENOTSUP;
	}
}

static int imx335_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct imx335_config *cfg = dev->config;
	struct imx335_data *drv_data = dev->data;
	int ret;

	struct video_frmival_enum match = {
		.format = &drv_data->fmt,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.discrete = *frmival,
	};
	video_closest_frmival(dev, &match);

	uint16_t hmax;

	switch (match.index) {
	case IMX335_25_FPS:
		hmax = (drv_data->fmt.width == IMX335_BIN_2X2_WIDTH
				&& drv_data->fmt.height == IMX335_BIN_2X2_HEIGHT) ? 0x0280 : 0x0294;
		break;
	case IMX335_30_FPS:
		hmax = 0x0226;
		break;
	case IMX335_50_FPS:
		hmax = 0x0140;
		break;
	case IMX335_60_FPS:
		hmax = 0x0113;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* use REGHOLD to ensure that our change is applied consistently */
	ret = video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX335_HMAX, hmax);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX335_REGHOLD, 0);
	if (ret < 0) {
		return ret;
	}

	frmival->numerator = 1;
	frmival->denominator = imx335_framerates[match.index];
	drv_data->frame_rate = imx335_framerates[match.index];

	return 0;
}

static int imx335_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct imx335_data *drv_data = dev->data;
	const struct imx335_config *cfg = dev->config;
	struct imx335_ctrls *ctrls = &drv_data->ctrls;
	int ret;
	size_t fmt_idx;

	if (drv_data->enabled) {
		LOG_ERR("Cannot set format while the stream is running");
		return -EBUSY;
	}

	ret = video_format_caps_index(imx335_fmts, fmt, &fmt_idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
	}

	if (fmt_idx == IMX335_RES_2592x1944) {
		/* Full resolution */

		ret = video_write_cci_multiregs(&cfg->i2c, imx335_bin_none,
						ARRAY_SIZE(imx335_bin_none));
		if (ret < 0) {
			LOG_ERR("Failed to disable binning");
			return ret;
		}

		ctrls->exposure.range.max = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN;
		ctrls->exposure.range.def = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN;
	} else if (fmt_idx == IMX335_RES_1296x972) {
		/* 2x2 binning mode */

		LOG_DBG("2x2 binning mode active");
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_bin_2x2,
						ARRAY_SIZE(imx335_bin_2x2));
		if (ret < 0) {
			LOG_ERR("Failed to enable binning");
			return ret;
		}

		ctrls->exposure.range.max = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN_BINNING;
		ctrls->exposure.range.def = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN_BINNING;

		/* update exposure, since the upper limit is lower if binning is active */
		ctrls->exposure.val = MIN(ctrls->exposure.range.max, ctrls->exposure.val);
		ret = imx335_set_ctrl_exposure(dev);
		if (ret < 0) {
			LOG_ERR("Failed to update exposure while enabling binning");
			return ret;
		}
	}

	drv_data->fmt.width = fmt->width;
	drv_data->fmt.height = fmt->height;
	/* update framerate, since the timing and allowed framerates may have changed */
	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = drv_data->frame_rate,
	};

	return imx335_set_frmival(dev, &frmival);
}

static int imx335_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct imx335_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = drv_data->frame_rate;

	return 0;
}

static int imx335_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	size_t fmt_idx;
	int ret;

	ret = video_format_caps_index(imx335_fmts, fie->format, &fmt_idx);
	if (ret < 0) {
		return ret;
	}

	if ((fmt_idx == IMX335_RES_2592x1944 && fie->index > IMX335_30_FPS)
	     || (fmt_idx == IMX335_RES_1296x972 && fie->index > IMX335_60_FPS)) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = imx335_framerates[fie->index];

	return 0;
}

static DEVICE_API(video, imx335_driver_api) = {
	.set_format = imx335_set_fmt,
	.get_format = imx335_get_fmt,
	.get_caps = imx335_get_caps,
	.set_stream = imx335_set_stream,
	.set_ctrl = imx335_set_ctrl,
	.set_frmival = imx335_set_frmival,
	.get_frmival = imx335_get_frmival,
	.enum_frmival = imx335_enum_frmival,
};

static int imx335_set_input_clk(const struct device *dev, uint32_t rate)
{
	const struct imx335_config *cfg = dev->config;
	int ret;

	switch (rate) {
	case MHZ(74):
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_inck_74mhz,
						ARRAY_SIZE(imx335_inck_74mhz));
		break;
	case MHZ(27):
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_inck_27mhz,
						ARRAY_SIZE(imx335_inck_27mhz));
		break;
	case MHZ(24):
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_inck_24mhz,
						ARRAY_SIZE(imx335_inck_24mhz));
		break;
	case MHZ(18):
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_inck_18mhz,
						ARRAY_SIZE(imx335_inck_18mhz));
		break;
	case MHZ(6):
		ret = video_write_cci_multiregs(&cfg->i2c, imx335_inck_6mhz,
						ARRAY_SIZE(imx335_inck_6mhz));
		break;
	default:
		LOG_ERR("Unsupported inck freq (%d)\n", rate);
		ret = -EINVAL;
	}

	return ret;
}

static int imx335_init_controls(const struct device *dev)
{
	int ret;
	struct imx335_data *drv_data = dev->data;
	struct imx335_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(
		&ctrls->gain, dev, VIDEO_CID_ANALOGUE_GAIN,
		(struct video_ctrl_range){
			.min = IMX335_GAIN_MIN,
			.max = IMX335_GAIN_MAX,
			.step = IMX335_GAIN_UNIT_MDB,
			.def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
		(struct video_ctrl_range){
			.min = 1,
			.max = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN,
			.step = 1,
			.def = IMX335_VMAX_DEFAULT - IMX335_SHR0_MIN});
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(
		&ctrls->pixel_rate, dev, VIDEO_CID_PIXEL_RATE,
		(struct video_ctrl_range){
			.min64 = IMX335_PIXEL_RATE,
			.max64 = IMX335_PIXEL_RATE,
			.step64 = 1,
			.def64 = IMX335_PIXEL_RATE});
}

static int imx335_init(const struct device *dev)
{
	const struct imx335_config *cfg = dev->config;
	struct imx335_data *drv_data = dev->data;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		/* Power up sequence */
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_NSEC(500)); /* Tlow */

		gpio_pin_set_dt(&cfg->reset_gpio, 0);

		k_sleep(K_USEC(600)); /* T4 */
	}
#endif

	/* Initialize register values */
	ret = video_write_cci_multiregs(&cfg->i2c, imx335_init_params,
					ARRAY_SIZE(imx335_init_params));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	/* Apply the fixed value registers */
	ret = video_write_cci_multiregs16(&cfg->i2c, imx335_fixed_regs,
					  ARRAY_SIZE(imx335_fixed_regs));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	ret = imx335_set_fmt(dev, &drv_data->fmt);
	if (ret < 0) {
		LOG_ERR("Unable to apply format");
		return ret;
	}

	/* TODO - Only 10bit - 2 data lanes mode is supported for the time being */
	ret = video_write_cci_multiregs(&cfg->i2c, imx335_mode_2l_10b,
					ARRAY_SIZE(imx335_mode_2l_10b));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	ret = imx335_set_input_clk(dev, cfg->input_clk);
	if (ret < 0) {
		LOG_ERR("Unable to configure INCK");
		return ret;
	}

	return imx335_init_controls(dev);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define IMX335_GET_RESET_GPIO(n)	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define IMX335_GET_RESET_GPIO(n)
#endif

#define IMX335_INIT(n)										\
	static struct imx335_data imx335_data_##n = {						\
		.fmt = {									\
			.pixelformat = VIDEO_PIX_FMT_SRGGB10P,					\
			.width = IMX335_NATIVE_WIDTH,						\
			.height = IMX335_NATIVE_HEIGHT,						\
		},										\
		.frame_rate = 30,								\
		.enabled = false,								\
	};											\
	static const struct imx335_config imx335_cfg_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		IMX335_GET_RESET_GPIO(n)							\
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &imx335_init, NULL, &imx335_data_##n, &imx335_cfg_##n,		\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &imx335_driver_api);	\
												\
	VIDEO_DEVICE_DEFINE(imx335_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(IMX335_INIT)
