/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sony_imx662

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_imx662, CONFIG_VIDEO_LOG_LEVEL);

#define IMX662_NATIVE_WIDTH	1936
#define IMX662_NATIVE_HEIGHT	1100

#define IMX662_2DL_LINK_FREQ 360000000
static const int64_t imx662_2dl_link_frequency[] = {
	IMX662_2DL_LINK_FREQ,
};

enum imx662_framerate {
	IMX662_1_FPS,
	IMX662_2_FPS,
	IMX662_5_FPS,
	IMX662_10_FPS,
	IMX662_15_FPS,
	IMX662_30_FPS,
};

struct imx662_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
	uint32_t input_clk;
};

struct imx662_ctrls {
	struct video_ctrl gain;
	struct video_ctrl exposure;
	struct video_ctrl link_freq;
};

struct imx662_data {
	struct imx662_ctrls ctrls;
	struct video_format fmt;
	uint32_t frame_rate;
	bool enabled;
};

#define IMX662_REG8(addr)  ((addr) | VIDEO_REG_ADDR16_DATA8)
#define IMX662_REG16(addr)  ((addr) | VIDEO_REG_ADDR16_DATA16_LE)
#define IMX662_REG24(addr)  ((addr) | VIDEO_REG_ADDR16_DATA24_LE)

#define IMX662_STANDBY			IMX662_REG8(0x3000)
#define IMX662_STANDBY_OPERATING	0x00
#define IMX662_STANDBY_STANDBY		BIT(0)
#define IMX662_REGHOLD			IMX662_REG8(0x3001)
#define IMX662_XMSTA			IMX662_REG8(0x3002)
#define IMX662_XMSTA_START		0x00
#define IMX662_XMSTA_STOP		BIT(0)
#define IMX662_INCK_SEL			IMX662_REG8(0x3014)
#define IMX662_INCK_SEL_24MHZ		0x04
#define IMX662_DATARATE_SEL		IMX662_REG8(0x3015)
#define IMX662_DATARATE_SEL_720MBPS	0x06
#define IMX662_WINMODE			IMX662_REG8(0x3018)
#define IMX662_WDMODE			IMX662_REG8(0x301a)
#define IMX662_ADDMODE			IMX662_REG8(0x301b)
#define IMX662_THIN_V_EN		IMX662_REG8(0x301c)
#define IMX662_VCMODE			IMX662_REG8(0x301e)
#define IMX662_HREVERSE			IMX662_REG8(0x3020)
#define IMX662_VREVERSE			IMX662_REG8(0x3021)
#define IMX662_ADBIT			IMX662_REG8(0x3022)
#define IMX662_MDBIT			IMX662_REG8(0x3023)
#define IMX662_VMAX			IMX662_REG24(0x3028)
#define IMX662_VMAX_DEFAULT		1250
#define IMX662_HMAX			IMX662_REG16(0x302c)
#define IMX662_FDG_SEL0			IMX662_REG8(0x3030)
#define IMX662_FDG_SEL1			IMX662_REG8(0x3031)
#define IMX662_LANEMODE			IMX662_REG8(0x3040)
#define IMX662_SHR0			IMX662_REG24(0x3050)
#define IMX662_SHR0_MIN			4
#define IMX662_GAIN			IMX662_REG16(0x3070)
#define IMX662_GAIN_MIN			(0 * 1000)
#define IMX662_GAIN_MAX			(72 * 1000)
#define IMX662_GAIN_UNIT_MDB		300
#define IMX662_XVS_XHS_OUTSEL		IMX662_REG8(0x30a4)
#define IMX662_XVS_XHS_DRV		IMX662_REG8(0x30a6)
#define IMX662_BLKLEVEL			IMX662_REG16(0x30dc)
#define IMX662_GAIN_PGC_FIDMD		IMX662_REG8(0x3400)

/* Delay between leaving standby and starting the master mode */
#define IMX662_STREAM_DELAY_MS		25

static const struct video_reg imx662_init_params[] = {
	{IMX662_STANDBY, 0x01},
	{IMX662_XMSTA, 0x01},
	{IMX662_REGHOLD, 0x00},
	{IMX662_WINMODE, 0x00},
	{IMX662_WDMODE, 0x00},
	{IMX662_ADDMODE, 0x00},
	{IMX662_THIN_V_EN, 0x00},
	{IMX662_VCMODE, 0x01},
	{IMX662_HREVERSE, 0x00},
	{IMX662_VREVERSE, 0x00},
	{IMX662_HMAX, 0x07bc},
	{IMX662_FDG_SEL0, 0x00},
	{IMX662_FDG_SEL1, 0x00},
	{IMX662_GAIN, 0x0000},
	{IMX662_XVS_XHS_OUTSEL, 0xaa},
	{IMX662_XVS_XHS_DRV, 0x00},
	{IMX662_BLKLEVEL, 0x4032},
	{IMX662_GAIN_PGC_FIDMD, 0x01},
};

static const struct video_reg16 imx662_fixed_regs[] = {
	{0x3444, 0xac},
	{0x3460, 0x21},
	{0x3492, 0x08},
	{0x3b00, 0x39},
	{0x3b23, 0x2d},
	{0x3b45, 0x04},
	{0x3c0a, 0x1f},
	{0x3c0b, 0x1e},
	{0x3c38, 0x21},
	{0x3c44, 0x00},
	{0x3cb6, 0xd8},
	{0x3cc4, 0xda},
	{0x3e24, 0x79},
	{0x3e2c, 0x15},
	{0x3edc, 0x2d},
	{0x4498, 0x05},
	{0x449c, 0x19},
	{0x449d, 0x00},
	{0x449e, 0x32},
	{0x449f, 0x01},
	{0x44a0, 0x92},
	{0x44a2, 0x91},
	{0x44a4, 0x8c},
	{0x44a6, 0x87},
	{0x44a8, 0x82},
	{0x44aa, 0x78},
	{0x44ac, 0x6e},
	{0x44ae, 0x69},
	{0x44b0, 0x92},
	{0x44b2, 0x91},
	{0x44b4, 0x8c},
	{0x44b6, 0x87},
	{0x44b8, 0x82},
	{0x44ba, 0x78},
	{0x44bc, 0x6e},
	{0x44be, 0x69},
	{0x44c0, 0x7f},
	{0x44c1, 0x01},
	{0x44c2, 0x7f},
	{0x44c3, 0x01},
	{0x44c4, 0x7a},
	{0x44c5, 0x01},
	{0x44c6, 0x7a},
	{0x44c7, 0x01},
	{0x44c8, 0x70},
	{0x44c9, 0x01},
	{0x44ca, 0x6b},
	{0x44cb, 0x01},
	{0x44cc, 0x6b},
	{0x44cd, 0x01},
	{0x44ce, 0x5c},
	{0x44cf, 0x01},
	{0x44d0, 0x7f},
	{0x44d1, 0x01},
	{0x44d2, 0x7f},
	{0x44d3, 0x01},
	{0x44d4, 0x7a},
	{0x44d5, 0x01},
	{0x44d6, 0x7a},
	{0x44d7, 0x01},
	{0x44d8, 0x70},
	{0x44d9, 0x01},
	{0x44da, 0x6b},
	{0x44db, 0x01},
	{0x44dc, 0x6b},
	{0x44dd, 0x01},
	{0x44de, 0x5c},
	{0x44df, 0x01},
	{0x4534, 0x1c},
	{0x4535, 0x03},
	{0x4538, 0x1c},
	{0x4539, 0x1c},
	{0x453a, 0x1c},
	{0x453b, 0x1c},
	{0x453c, 0x1c},
	{0x453d, 0x1c},
	{0x453e, 0x1c},
	{0x453f, 0x1c},
	{0x4540, 0x1c},
	{0x4541, 0x03},
	{0x4542, 0x03},
	{0x4543, 0x03},
	{0x4544, 0x03},
	{0x4545, 0x03},
	{0x4546, 0x03},
	{0x4547, 0x03},
	{0x4548, 0x03},
	{0x4549, 0x03},
};

static const struct video_reg imx662_mode_2l_12b[] = {
	{IMX662_ADBIT, 0x01},
	{IMX662_MDBIT, 0x01},
	{IMX662_REG24(0x3a50), 0x0003ff},
	{IMX662_DATARATE_SEL, IMX662_DATARATE_SEL_720MBPS},
	{IMX662_LANEMODE, 0x01},
};

static const struct video_reg imx662_inck_24mhz[] = {
	{IMX662_INCK_SEL, IMX662_INCK_SEL_24MHZ},
};

static const struct video_format_cap imx662_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_SRGGB12P,
		.width_min = IMX662_NATIVE_WIDTH,
		.width_max = IMX662_NATIVE_WIDTH,
		.height_min = IMX662_NATIVE_HEIGHT,
		.height_max = IMX662_NATIVE_HEIGHT,
		.width_step = 0,
		.height_step = 0,
	},
	{0}
};

static const uint32_t imx662_framerates[] = {
	[IMX662_1_FPS] = 1,
	[IMX662_2_FPS] = 2,
	[IMX662_5_FPS] = 5,
	[IMX662_10_FPS] = 10,
	[IMX662_15_FPS] = 15,
	[IMX662_30_FPS] = 30,
};

/* The line length is fixed, the frame rate is set by the number of lines per frame */
static uint32_t imx662_get_vmax(uint32_t frame_rate)
{
	return IMX662_VMAX_DEFAULT * 30 / frame_rate;
}

static uint32_t imx662_get_shr0(uint32_t vmax, uint32_t exposure)
{
	return vmax - exposure;
}

static int imx662_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct imx662_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int imx662_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = imx662_fmts;
	return 0;
}

static int imx662_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct imx662_config *cfg = dev->config;
	struct imx662_data *drv_data = dev->data;
	int ret;

	if (enable) {
		ret = video_write_cci_reg(&cfg->i2c, IMX662_STANDBY, IMX662_STANDBY_OPERATING);
		if (ret < 0) {
			LOG_ERR("Failed to set standby register");
			return ret;
		}

		k_sleep(K_MSEC(IMX662_STREAM_DELAY_MS));

		ret = video_write_cci_reg(&cfg->i2c, IMX662_XMSTA, IMX662_XMSTA_START);
	} else {
		ret = video_write_cci_reg(&cfg->i2c, IMX662_STANDBY, IMX662_STANDBY_STANDBY);
		if (ret < 0) {
			LOG_ERR("Failed to set standby register");
			return ret;
		}

		ret = video_write_cci_reg(&cfg->i2c, IMX662_XMSTA, IMX662_XMSTA_STOP);
	}
	if (ret < 0) {
		LOG_ERR("Failed to set master mode register");
		return ret;
	}

	drv_data->enabled = enable;

	return 0;
}

static int imx662_set_ctrl_gain(const struct device *dev)
{
	const struct imx662_config *cfg = dev->config;
	struct imx662_data *drv_data = dev->data;
	struct imx662_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	/* Apply gain upon conversion to gain unit */
	ret = video_write_cci_reg(&cfg->i2c, IMX662_GAIN, ctrls->gain.val / IMX662_GAIN_UNIT_MDB);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 0);
}

static int imx662_set_ctrl_exposure(const struct device *dev)
{
	const struct imx662_config *cfg = dev->config;
	struct imx662_data *drv_data = dev->data;
	struct imx662_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX662_SHR0,
				  imx662_get_shr0(imx662_get_vmax(drv_data->frame_rate),
						  ctrls->exposure.val));
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 0);
}

static int imx662_set_ctrl(const struct device *dev, unsigned int cid)
{
	switch (cid) {
	case VIDEO_CID_ANALOGUE_GAIN:
		return imx662_set_ctrl_gain(dev);
	case VIDEO_CID_EXPOSURE:
		return imx662_set_ctrl_exposure(dev);
	default:
		return -ENOTSUP;
	}
}

static int imx662_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct imx662_config *cfg = dev->config;
	struct imx662_data *drv_data = dev->data;
	struct imx662_ctrls *ctrls = &drv_data->ctrls;
	uint32_t vmax;
	int ret;

	struct video_frmival_enum match = {
		.format = &drv_data->fmt,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.discrete = *frmival,
	};
	ret = video_closest_frmival(dev, &match);
	if (ret < 0) {
		return ret;
	}

	vmax = imx662_get_vmax(imx662_framerates[match.index]);

	/* update exposure, since the upper limit follows the number of lines per frame */
	ctrls->exposure.range.max = vmax - IMX662_SHR0_MIN;
	ctrls->exposure.val = MIN(ctrls->exposure.range.max, ctrls->exposure.val);

	/* use REGHOLD to ensure that our change is applied consistently */
	ret = video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 1);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX662_VMAX, vmax);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX662_SHR0,
				  imx662_get_shr0(vmax, ctrls->exposure.val));
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, IMX662_REGHOLD, 0);
	if (ret < 0) {
		return ret;
	}

	frmival->numerator = 1;
	frmival->denominator = imx662_framerates[match.index];
	drv_data->frame_rate = imx662_framerates[match.index];

	return 0;
}

static int imx662_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct imx662_data *drv_data = dev->data;
	int ret;
	size_t fmt_idx;

	if (drv_data->enabled) {
		LOG_ERR("Cannot set format while the stream is running");
		return -EBUSY;
	}

	ret = video_format_caps_index(imx662_fmts, fmt, &fmt_idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
	}

	drv_data->fmt.width = fmt->width;
	drv_data->fmt.height = fmt->height;
	/* apply the frame rate */
	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = drv_data->frame_rate,
	};

	return imx662_set_frmival(dev, &frmival);
}

static int imx662_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct imx662_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = drv_data->frame_rate;

	return 0;
}

static int imx662_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	size_t fmt_idx;
	int ret;

	ret = video_format_caps_index(imx662_fmts, fie->format, &fmt_idx);
	if (ret < 0) {
		return ret;
	}

	if (fie->index > IMX662_30_FPS) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = imx662_framerates[fie->index];

	return 0;
}

static DEVICE_API(video, imx662_driver_api) = {
	.set_format = imx662_set_fmt,
	.get_format = imx662_get_fmt,
	.get_caps = imx662_get_caps,
	.set_stream = imx662_set_stream,
	.set_ctrl = imx662_set_ctrl,
	.set_frmival = imx662_set_frmival,
	.get_frmival = imx662_get_frmival,
	.enum_frmival = imx662_enum_frmival,
};

static int imx662_set_input_clk(const struct device *dev, uint32_t rate)
{
	const struct imx662_config *cfg = dev->config;
	int ret;

	switch (rate) {
	case MHZ(24):
		ret = video_write_cci_multiregs(&cfg->i2c, imx662_inck_24mhz,
						ARRAY_SIZE(imx662_inck_24mhz));
		break;
	default:
		LOG_ERR("Unsupported inck freq (%u)", rate);
		ret = -EINVAL;
	}

	return ret;
}

static int imx662_init_controls(const struct device *dev)
{
	int ret;
	struct imx662_data *drv_data = dev->data;
	struct imx662_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(
		&ctrls->gain, dev, VIDEO_CID_ANALOGUE_GAIN,
		(struct video_ctrl_range){
			.min = IMX662_GAIN_MIN,
			.max = IMX662_GAIN_MAX,
			.step = IMX662_GAIN_UNIT_MDB,
			.def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
		(struct video_ctrl_range){
			.min = 1,
			.max = IMX662_VMAX_DEFAULT - IMX662_SHR0_MIN,
			.step = 1,
			.def = IMX662_VMAX_DEFAULT - IMX662_SHR0_MIN});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->link_freq, dev, VIDEO_CID_LINK_FREQ, 0,
				       imx662_2dl_link_frequency,
				       ARRAY_SIZE(imx662_2dl_link_frequency));
	if (ret < 0) {
		return ret;
	}
	ctrls->link_freq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

static int imx662_init(const struct device *dev)
{
	const struct imx662_config *cfg = dev->config;
	struct imx662_data *drv_data = dev->data;
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
	ret = video_write_cci_multiregs(&cfg->i2c, imx662_init_params,
					ARRAY_SIZE(imx662_init_params));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	/* Apply the fixed value registers */
	ret = video_write_cci_multiregs16(&cfg->i2c, imx662_fixed_regs,
					  ARRAY_SIZE(imx662_fixed_regs));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	ret = imx662_init_controls(dev);
	if (ret < 0) {
		return ret;
	}

	ret = imx662_set_fmt(dev, &drv_data->fmt);
	if (ret < 0) {
		LOG_ERR("Unable to apply format");
		return ret;
	}

	/* TODO - Only 12bit - 2 data lanes mode is supported for the time being */
	ret = video_write_cci_multiregs(&cfg->i2c, imx662_mode_2l_12b,
					ARRAY_SIZE(imx662_mode_2l_12b));
	if (ret < 0) {
		LOG_ERR("Unable to initialize the sensor");
		return ret;
	}

	ret = imx662_set_input_clk(dev, cfg->input_clk);
	if (ret < 0) {
		LOG_ERR("Unable to configure INCK");
		return ret;
	}

	return 0;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define IMX662_GET_RESET_GPIO(n)	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define IMX662_GET_RESET_GPIO(n)
#endif

#define IMX662_INIT(n)										\
	static struct imx662_data imx662_data_##n = {						\
		.fmt = {									\
			.pixelformat = VIDEO_PIX_FMT_SRGGB12P,					\
			.width = IMX662_NATIVE_WIDTH,						\
			.height = IMX662_NATIVE_HEIGHT,						\
		},										\
		.frame_rate = 30,								\
		.enabled = false,								\
	};											\
	static const struct imx662_config imx662_cfg_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		IMX662_GET_RESET_GPIO(n)							\
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency),		\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, &imx662_init, NULL, &imx662_data_##n, &imx662_cfg_##n,		\
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &imx662_driver_api);	\
												\
	VIDEO_DEVICE_DEFINE(imx662_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(IMX662_INIT)
