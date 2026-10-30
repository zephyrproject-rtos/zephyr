/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov5647

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_ov5647, CONFIG_VIDEO_LOG_LEVEL);

#define OV5647_CHIP_ID			0x5647

#define OV5647_FULL_WIDTH		2592
#define OV5647_FULL_HEIGHT		1944

/* Position of the first output pixel in the pixel array, from the power-on window */
#define OV5647_X_ADDR_START		12
#define OV5647_Y_ADDR_START		4

/*
 * The read-out window is wider and taller than the output window by this many pixels, which the
 * ISP crops back off using the offsets at 0x3810..0x3813 left at their power-on values.
 */
#define OV5647_WINDOW_MARGIN		8

/* Line length, kept at its power-on value whatever the output size is */
#define OV5647_HTS			2700
/* Power-on frame length minus the full output height */
#define OV5647_VBLANK_MIN		24

/*
 * The PLL is left in its power-on configuration, where the pixel rate is a fixed multiple of
 * XVCLK. Datasheet table 2-1 gives 15 fps at full resolution, which with the power-on HTS and VTS
 * is the 80 MHz that this ratio yields from the nominal 24 MHz input clock.
 */
#define OV5647_PIXEL_RATE(clk)		((clk) * 10 / 3)
#define OV5647_INPUT_CLK_MIN		MHZ(6)
#define OV5647_INPUT_CLK_MAX		MHZ(27)

#define OV5647_REG8(addr)		((addr) | VIDEO_REG_ADDR16_DATA8)
#define OV5647_REG16(addr)		((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define OV5647_REG24(addr)		((addr) | VIDEO_REG_ADDR16_DATA24_BE)

#define OV5647_MODE_SELECT		OV5647_REG8(0x0100)
#define OV5647_MODE_SELECT_STREAMING	BIT(0)
#define OV5647_SOFTWARE_RESET		OV5647_REG8(0x0103)
#define OV5647_SOFTWARE_RESET_RESET	BIT(0)
#define OV5647_CHIP_ID_REG		OV5647_REG16(0x300a)
#define OV5647_SC_MIPI_PHY		OV5647_REG8(0x3016)
#define OV5647_MIPI_PAD_ENABLE		BIT(3)
#define OV5647_SC_MIPI_SC_CTRL		OV5647_REG8(0x3018)
#define OV5647_PHY_PD_MIPI		BIT(4)
#define OV5647_PHY_PD_LPRX		BIT(3)
#define OV5647_MIPI_EN			BIT(2)
#define OV5647_SC_PLL_CTRL0		OV5647_REG8(0x3034)
#define OV5647_MIPI_BIT_MODE		GENMASK(3, 0)
#define OV5647_EXPOSURE			OV5647_REG24(0x3500)
#define OV5647_EXPOSURE_MAX		GENMASK(19, 0)
#define OV5647_EXPOSURE_DEFAULT		0x20
#define OV5647_MANUAL_CTRL		OV5647_REG8(0x3503)
#define OV5647_MANUAL_CTRL_VTS		BIT(2)
#define OV5647_MANUAL_CTRL_AGC		BIT(1)
#define OV5647_MANUAL_CTRL_AEC		BIT(0)
#define OV5647_AGC_GAIN			OV5647_REG16(0x350a)
#define OV5647_AGC_GAIN_MAX		GENMASK(9, 0)
#define OV5647_VTS_DIFF			OV5647_REG16(0x350c)
#define OV5647_TIMING_X_ADDR_START	OV5647_REG16(0x3800)
#define OV5647_TIMING_Y_ADDR_START	OV5647_REG16(0x3802)
#define OV5647_TIMING_X_ADDR_END	OV5647_REG16(0x3804)
#define OV5647_TIMING_Y_ADDR_END	OV5647_REG16(0x3806)
#define OV5647_TIMING_X_OUTPUT_SIZE	OV5647_REG16(0x3808)
#define OV5647_TIMING_Y_OUTPUT_SIZE	OV5647_REG16(0x380a)
#define OV5647_TIMING_HTS_REG		OV5647_REG16(0x380c)
#define OV5647_TIMING_VTS_REG		OV5647_REG16(0x380e)
#define OV5647_TIMING_TC_REG20		OV5647_REG8(0x3820)
#define OV5647_TC_REG20_VFLIP		(BIT(2) | BIT(1))
#define OV5647_TIMING_TC_REG21		OV5647_REG8(0x3821)
#define OV5647_TC_REG21_MIRROR		(BIT(2) | BIT(1))
#define OV5647_ISP_CTRL3D		OV5647_REG8(0x503d)
#define OV5647_TEST_PATTERN_ENABLE	BIT(7)

/*
 * Datasheet table 7-1 describes the 0x3034 bit mode field as 0 for 8-bit and 1 for 10-bit, which
 * does not match its own 0x1A power-on value for this 10-bit sensor: the field holds the number of
 * bits, as it does on the rest of the family.
 */
#define OV5647_MIPI_BIT_MODE_RAW8	8
#define OV5647_MIPI_BIT_MODE_RAW10	10

struct ov5647_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn_gpio;
#endif
	uint32_t pixel_rate;
};

struct ov5647_ctrls {
	/* auto_gain and gain are clustered together and must stay adjacent */
	struct video_ctrl auto_gain;
	struct video_ctrl gain;
	struct video_ctrl exposure_auto;
	struct video_ctrl exposure;
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl test_pattern;
	struct video_ctrl pixel_rate;
};

struct ov5647_data {
	struct ov5647_ctrls ctrls;
	struct video_format fmt;
	uint32_t frmrate;
	bool streaming;
};

static const struct video_reg ov5647_init_regs[] = {
	{OV5647_SC_MIPI_PHY, OV5647_MIPI_PAD_ENABLE},
	/* Drive the frame length from TIMING_VTS instead of letting the AEC stretch it */
	{OV5647_MANUAL_CTRL, OV5647_MANUAL_CTRL_VTS},
	{OV5647_VTS_DIFF, 0},
	{OV5647_TIMING_HTS_REG, OV5647_HTS},
};

enum ov5647_fmt_id {
	OV5647_FMT_SBGGR8,
	OV5647_FMT_SBGGR10P,
};

static const struct video_format_cap ov5647_fmts[] = {
	[OV5647_FMT_SBGGR8] = {
		.pixelformat = VIDEO_PIX_FMT_SBGGR8,
		.width_min = 4, .width_max = OV5647_FULL_WIDTH, .width_step = 4,
		.height_min = 4, .height_max = OV5647_FULL_HEIGHT, .height_step = 4,
	},
	[OV5647_FMT_SBGGR10P] = {
		.pixelformat = VIDEO_PIX_FMT_SBGGR10P,
		.width_min = 4, .width_max = OV5647_FULL_WIDTH, .width_step = 4,
		.height_min = 4, .height_max = OV5647_FULL_HEIGHT, .height_step = 4,
	},
	{0},
};

/* Frame rates from datasheet table 2-1, plus a slower one usable at any resolution */
static const uint32_t ov5647_framerates[] = {10, 15, 30, 45, 60, 90, 120};

static uint32_t ov5647_frmrate_to_vts(const struct device *dev, uint32_t frmrate)
{
	const struct ov5647_config *cfg = dev->config;

	return cfg->pixel_rate / (OV5647_HTS * frmrate);
}

static int ov5647_set_window(const struct device *dev, uint32_t width, uint32_t height)
{
	const struct ov5647_config *cfg = dev->config;
	uint32_t x_start = OV5647_X_ADDR_START + (OV5647_FULL_WIDTH - width) / 2;
	uint32_t y_start = OV5647_Y_ADDR_START + (OV5647_FULL_HEIGHT - height) / 2;
	const struct video_reg regs[] = {
		{OV5647_TIMING_X_ADDR_START, x_start},
		{OV5647_TIMING_Y_ADDR_START, y_start},
		{OV5647_TIMING_X_ADDR_END, x_start + width + OV5647_WINDOW_MARGIN - 1},
		{OV5647_TIMING_Y_ADDR_END, y_start + height + OV5647_WINDOW_MARGIN - 1},
		{OV5647_TIMING_X_OUTPUT_SIZE, width},
		{OV5647_TIMING_Y_OUTPUT_SIZE, height},
	};

	return video_write_cci_multiregs(&cfg->i2c, regs, ARRAY_SIZE(regs));
}

static int ov5647_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (fie->index >= ARRAY_SIZE(ov5647_framerates)) {
		return -EINVAL;
	}

	/* A frame rate is only reachable if its frame length still covers the read-out */
	if (ov5647_frmrate_to_vts(dev, ov5647_framerates[fie->index]) <
	    fie->format->height + OV5647_VBLANK_MIN) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = ov5647_framerates[fie->index];

	return 0;
}

static int ov5647_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct video_frmival_enum fie = {
		.discrete = *frmival,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.format = &data->fmt,
	};
	int ret;

	ret = video_closest_frmival(dev, &fie);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, OV5647_TIMING_VTS_REG,
				  ov5647_frmrate_to_vts(dev, ov5647_framerates[fie.index]));
	if (ret < 0) {
		return ret;
	}

	*frmival = fie.discrete;
	data->frmrate = ov5647_framerates[fie.index];

	return 0;
}

static int ov5647_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ov5647_data *data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = data->frmrate;

	return 0;
}

static int ov5647_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct video_frmival frmival = {.numerator = 1, .denominator = data->frmrate};
	size_t idx;
	int ret;

	if (data->streaming) {
		LOG_ERR("Cannot change the format while streaming");
		return -EBUSY;
	}

	ret = video_format_caps_index(ov5647_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format '%s' %ux%u not supported", VIDEO_FOURCC_TO_STR(fmt->pixelformat),
			fmt->width, fmt->height);
		return -ENOTSUP;
	}

	/* Centering the window on the pixel array must not shift the Bayer order */
	if (fmt->width % ov5647_fmts[idx].width_step != 0 ||
	    fmt->height % ov5647_fmts[idx].height_step != 0) {
		LOG_ERR("Resolution %ux%u is not a multiple of %ux%u", fmt->width, fmt->height,
			ov5647_fmts[idx].width_step, ov5647_fmts[idx].height_step);
		return -EINVAL;
	}

	ret = video_modify_cci_reg(&cfg->i2c, OV5647_SC_PLL_CTRL0, OV5647_MIPI_BIT_MODE,
				   idx == OV5647_FMT_SBGGR8 ? OV5647_MIPI_BIT_MODE_RAW8
							    : OV5647_MIPI_BIT_MODE_RAW10);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_window(dev, fmt->width, fmt->height);
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;

	/* The reachable frame rates depend on the height that was just programmed */
	return ov5647_set_frmival(dev, &frmival);
}

static int ov5647_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov5647_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

static int ov5647_get_caps(const struct device *dev, struct video_caps *caps)
{
	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

	caps->format_caps = ov5647_fmts;

	return 0;
}

static int ov5647_set_stream(const struct device *dev, bool on, enum video_buf_type type)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	int ret;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

	ret = video_write_cci_reg(&cfg->i2c, OV5647_MODE_SELECT,
				  on ? OV5647_MODE_SELECT_STREAMING : 0);
	if (ret < 0) {
		return ret;
	}

	data->streaming = on;

	return 0;
}

static int ov5647_set_ctrl_gain(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct ov5647_ctrls *ctrls = &data->ctrls;
	int ret;

	ret = video_modify_cci_reg(&cfg->i2c, OV5647_MANUAL_CTRL, OV5647_MANUAL_CTRL_AGC,
				   ctrls->auto_gain.val != 0 ? 0 : OV5647_MANUAL_CTRL_AGC);
	if (ret < 0) {
		return ret;
	}

	if (ctrls->auto_gain.val != 0) {
		return 0;
	}

	return video_write_cci_reg(&cfg->i2c, OV5647_AGC_GAIN, ctrls->gain.val);
}

static int ov5647_set_ctrl_exposure(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct ov5647_ctrls *ctrls = &data->ctrls;
	int ret;

	ret = video_modify_cci_reg(&cfg->i2c, OV5647_MANUAL_CTRL, OV5647_MANUAL_CTRL_AEC,
				   ctrls->exposure_auto.val == VIDEO_EXPOSURE_MANUAL
					   ? OV5647_MANUAL_CTRL_AEC
					   : 0);
	if (ret < 0) {
		return ret;
	}

	if (ctrls->exposure_auto.val != VIDEO_EXPOSURE_MANUAL) {
		return 0;
	}

	return video_write_cci_reg(&cfg->i2c, OV5647_EXPOSURE, ctrls->exposure.val);
}

static int ov5647_set_ctrl_test_pattern(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	int32_t val = data->ctrls.test_pattern.val;

	return video_write_cci_reg(&cfg->i2c, OV5647_ISP_CTRL3D,
				   val == 0 ? 0 : (OV5647_TEST_PATTERN_ENABLE | (val - 1)));
}

static int ov5647_set_ctrl(const struct device *dev, uint32_t cid)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct ov5647_ctrls *ctrls = &data->ctrls;

	switch (cid) {
	case VIDEO_CID_AUTOGAIN:
		return ov5647_set_ctrl_gain(dev);
	case VIDEO_CID_EXPOSURE_AUTO:
	case VIDEO_CID_EXPOSURE:
		return ov5647_set_ctrl_exposure(dev);
	case VIDEO_CID_HFLIP:
		return video_modify_cci_reg(&cfg->i2c, OV5647_TIMING_TC_REG21,
					    OV5647_TC_REG21_MIRROR,
					    ctrls->hflip.val != 0 ? OV5647_TC_REG21_MIRROR : 0);
	case VIDEO_CID_VFLIP:
		return video_modify_cci_reg(&cfg->i2c, OV5647_TIMING_TC_REG20,
					    OV5647_TC_REG20_VFLIP,
					    ctrls->vflip.val != 0 ? OV5647_TC_REG20_VFLIP : 0);
	case VIDEO_CID_TEST_PATTERN:
		return ov5647_set_ctrl_test_pattern(dev);
	default:
		return -ENOTSUP;
	}
}

static int ov5647_get_volatile_ctrl(const struct device *dev, uint32_t cid)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	uint32_t gain;
	int ret;

	if (cid != VIDEO_CID_AUTOGAIN) {
		return -ENOTSUP;
	}

	ret = video_read_cci_reg(&cfg->i2c, OV5647_AGC_GAIN, &gain);
	if (ret < 0) {
		return ret;
	}

	data->ctrls.gain.val = gain & OV5647_AGC_GAIN_MAX;

	return 0;
}

static DEVICE_API(video, ov5647_driver_api) = {
	.set_format = ov5647_set_fmt,
	.get_format = ov5647_get_fmt,
	.get_caps = ov5647_get_caps,
	.set_stream = ov5647_set_stream,
	.set_ctrl = ov5647_set_ctrl,
	.get_volatile_ctrl = ov5647_get_volatile_ctrl,
	.set_frmival = ov5647_set_frmival,
	.get_frmival = ov5647_get_frmival,
	.enum_frmival = ov5647_enum_frmival,
};

static const char *const ov5647_exposure_auto_menu[] = {
	"Auto Mode",
	"Manual Mode",
	NULL,
};

static const char *const ov5647_test_pattern_menu[] = {
	"Off",
	"Color bar",
	"Color square",
	"Random data",
	NULL,
};

static int ov5647_init_ctrls(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *data = dev->data;
	struct ov5647_ctrls *ctrls = &data->ctrls;
	int ret;

	ret = video_init_ctrl(&ctrls->auto_gain, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->gain, dev, VIDEO_CID_ANALOGUE_GAIN,
			      (struct video_ctrl_range){.min = 0, .max = OV5647_AGC_GAIN_MAX,
							.step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_auto_cluster_ctrl(&ctrls->auto_gain, 2, true);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->exposure_auto, dev, VIDEO_CID_EXPOSURE_AUTO,
				   VIDEO_EXPOSURE_AUTO, ov5647_exposure_auto_menu);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
			      (struct video_ctrl_range){.min = 0, .max = OV5647_EXPOSURE_MAX,
							.step = 1,
							.def = OV5647_EXPOSURE_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				   ov5647_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(&ctrls->pixel_rate, dev, VIDEO_CID_PIXEL_RATE,
			       (struct video_ctrl_range){.min64 = cfg->pixel_rate,
							 .max64 = cfg->pixel_rate,
							 .step64 = 1,
							 .def64 = cfg->pixel_rate});
}

static int ov5647_init(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_SBGGR10P,
		.width = OV5647_FULL_WIDTH,
		.height = OV5647_FULL_HEIGHT,
	};
	uint32_t chip_id;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	if (cfg->pwdn_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->pwdn_gpio)) {
			LOG_ERR("Device %s is not ready", cfg->pwdn_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->pwdn_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		/* Datasheet section 2.5: the supplies must be stable before PWDN is released */
		k_sleep(K_MSEC(5));

		gpio_pin_set_dt(&cfg->pwdn_gpio, 0);

		/* Datasheet section 2.5: SCCB is only accessible 20 ms after PWDN goes low */
		k_sleep(K_MSEC(20));
	}
#endif

	ret = video_write_cci_reg(&cfg->i2c, OV5647_SOFTWARE_RESET, OV5647_SOFTWARE_RESET_RESET);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(5));

	ret = video_read_cci_reg(&cfg->i2c, OV5647_CHIP_ID_REG, &chip_id);
	if (ret < 0) {
		return ret;
	}

	if (chip_id != OV5647_CHIP_ID) {
		LOG_ERR("Wrong chip ID 0x%04x instead of 0x%04x", chip_id, OV5647_CHIP_ID);
		return -ENODEV;
	}

	ret = video_write_cci_multiregs(&cfg->i2c, ov5647_init_regs,
					ARRAY_SIZE(ov5647_init_regs));
	if (ret < 0) {
		return ret;
	}

	/* Route the pixel stream to the MIPI transmitter, keeping the power-on lane count */
	ret = video_modify_cci_reg(&cfg->i2c, OV5647_SC_MIPI_SC_CTRL,
				   OV5647_PHY_PD_MIPI | OV5647_PHY_PD_LPRX | OV5647_MIPI_EN,
				   OV5647_MIPI_EN);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	return ov5647_init_ctrls(dev);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define OV5647_GET_PWDN_GPIO(n) .pwdn_gpio = GPIO_DT_SPEC_INST_GET_OR(n, pwdn_gpios, {0}),
#else
#define OV5647_GET_PWDN_GPIO(n)
#endif

#define OV5647_EP(n) DT_CHILD(DT_INST_CHILD(n, port), endpoint)
#define OV5647_INPUT_CLK(n) DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency)

#define OV5647_INIT(n)                                                                             \
	BUILD_ASSERT(DT_PROP_OR(OV5647_EP(n), bus_type, VIDEO_BUS_TYPE_CSI2_DPHY) ==               \
			     VIDEO_BUS_TYPE_CSI2_DPHY,                                             \
		     "Only the MIPI CSI-2 D-PHY interface is supported");                          \
	BUILD_ASSERT(DT_PROP_LEN_OR(OV5647_EP(n), data_lanes, 2) == 2,                             \
		     "Only the two data lanes mode is supported");                                 \
	BUILD_ASSERT(IN_RANGE(OV5647_INPUT_CLK(n), OV5647_INPUT_CLK_MIN, OV5647_INPUT_CLK_MAX),    \
		     "XVCLK must be between 6 MHz and 27 MHz");                                    \
                                                                                                   \
	static struct ov5647_data ov5647_data_##n = {                                              \
		.frmrate = 15,                                                                     \
	};                                                                                         \
                                                                                                   \
	static const struct ov5647_config ov5647_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		OV5647_GET_PWDN_GPIO(n)                                                            \
		.pixel_rate = OV5647_PIXEL_RATE(OV5647_INPUT_CLK(n)),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ov5647_init, NULL, &ov5647_data_##n, &ov5647_cfg_##n,            \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov5647_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(ov5647_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV5647_INIT)
