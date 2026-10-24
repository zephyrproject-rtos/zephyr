/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov5647

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(ov5647, CONFIG_VIDEO_LOG_LEVEL);

#define OV5647_CHIP_ID			0x5647

/*
 * The pixel array is larger than the area the base configuration below reads out: it
 * places the active area at an offset of 16 columns and 6 rows, leaving that same
 * margin on the other side, which the crop window has to account for.
 */
#define OV5647_NATIVE_WIDTH		2624
#define OV5647_NATIVE_HEIGHT		1956
#define OV5647_ACTIVE_WIDTH		2592
#define OV5647_ACTIVE_HEIGHT		1944
#define OV5647_MARGIN_WIDTH		(OV5647_NATIVE_WIDTH - OV5647_ACTIVE_WIDTH)
#define OV5647_MARGIN_HEIGHT		(OV5647_NATIVE_HEIGHT - OV5647_ACTIVE_HEIGHT)

/*
 * Line length of the base configuration, from which the frame length achieving a
 * requested frame rate is derived, plus the shortest vertical blanking the sensor
 * tolerates and the longest frame length it can be programmed with.
 */
#define OV5647_LINE_LENGTH		2844
#define OV5647_VBLANK_MIN		24
#define OV5647_VTS_MAX			32767

/*
 * The base configuration multiplies the input clock by 105 and divides it by the
 * pre-divider of 3, the system divider of 2 and the DDR factor of 2, so the lanes
 * carry 105 / 12 of the input clock. The pre-divider is the reset value of 0x3037,
 * which the register table deliberately leaves alone: the resulting rate is what
 * the pixel rate the vendor documents for this configuration works out to.
 *
 * The pixel rate follows from the link frequency, since the two lanes carry ten
 * bits per pixel at twice the link frequency.
 */
#define OV5647_PLL_MULT			105
#define OV5647_PLL_DIV			12
#define OV5647_BITS_PER_PIXEL		10
#define OV5647_DATA_LANES		2

#define OV5647_LINK_FREQ_HZ(clk)                                                                   \
	((int64_t)(clk) * OV5647_PLL_MULT / OV5647_PLL_DIV)
#define OV5647_PIXEL_RATE_HZ(clk)                                                                  \
	(OV5647_LINK_FREQ_HZ(clk) * 2 * OV5647_DATA_LANES / OV5647_BITS_PER_PIXEL)

/* Time the sensor needs after being released from power down before it answers */
#define OV5647_PWDN_DELAY_MS		20

#define OV5647_REG8(addr)		((addr) | VIDEO_REG_ADDR16_DATA8)
#define OV5647_REG16(addr)		((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define OV5647_REG24(addr)		((addr) | VIDEO_REG_ADDR16_DATA24_BE)

#define OV5647_CCI_SW_STANDBY		OV5647_REG8(0x0100)
#define OV5647_SW_STANDBY_STREAMING	0x01
#define OV5647_SW_STANDBY_STANDBY	0x00
#define OV5647_CCI_SW_RESET		OV5647_REG8(0x0103)
#define OV5647_CCI_CHIP_ID		OV5647_REG16(0x300a)
#define OV5647_CCI_PAD_OUT		OV5647_REG8(0x300d)
#define OV5647_CCI_EXPOSURE		OV5647_REG24(0x3500)
#define OV5647_CCI_AEC_AGC		OV5647_REG8(0x3503)
#define OV5647_AEC_MANUAL		BIT(0)
#define OV5647_AGC_MANUAL		BIT(1)
#define OV5647_CCI_GAIN			OV5647_REG16(0x350a)
#define OV5647_CCI_X_ADDR_START		OV5647_REG16(0x3800)
#define OV5647_CCI_Y_ADDR_START		OV5647_REG16(0x3802)
#define OV5647_CCI_X_ADDR_END		OV5647_REG16(0x3804)
#define OV5647_CCI_Y_ADDR_END		OV5647_REG16(0x3806)
#define OV5647_CCI_X_OUTPUT_SIZE	OV5647_REG16(0x3808)
#define OV5647_CCI_Y_OUTPUT_SIZE	OV5647_REG16(0x380a)
#define OV5647_CCI_VTS			OV5647_REG16(0x380e)
#define OV5647_CCI_TIMING_TC_REG20	OV5647_REG8(0x3820)
#define OV5647_CCI_TIMING_TC_REG21	OV5647_REG8(0x3821)
#define OV5647_TIMING_TC_FLIP		GENMASK(2, 1)
#define OV5647_CCI_FRAME_OFF_NUMBER	OV5647_REG8(0x4202)
#define OV5647_CCI_MIPI_CTRL00		OV5647_REG8(0x4800)
#define OV5647_MIPI_CTRL00_CLOCK_LANE_GATE	BIT(5)
#define OV5647_MIPI_CTRL00_LINE_SYNC_ENABLE	BIT(4)
#define OV5647_MIPI_CTRL00_BUS_IDLE		BIT(2)
#define OV5647_MIPI_CTRL00_CLOCK_LANE_DISABLE	BIT(0)
#define OV5647_CCI_MIPI_CTRL14		OV5647_REG8(0x4814)
#define OV5647_MIPI_CTRL14_VC		GENMASK(7, 6)
#define OV5647_CCI_AWB			OV5647_REG8(0x5001)
#define OV5647_AWB_ENABLE		BIT(0)
#define OV5647_CCI_ISPCTRL3D		OV5647_REG8(0x503d)

/*
 * The exposure time is programmed in units of a sixteenth of a line, of which only
 * whole lines are exposed here, and it has to leave a few lines of every frame for
 * the sensor itself.
 */
#define OV5647_EXPOSURE_SHIFT		4
#define OV5647_EXPOSURE_MARGIN		4
#define OV5647_EXPOSURE_MIN		4
#define OV5647_EXPOSURE_DEFAULT		1000

/* Analog gain in sixteenths, so the reset value of 16 is unity */
#define OV5647_GAIN_MIN			16
#define OV5647_GAIN_MAX			1023
#define OV5647_GAIN_DEFAULT		32

struct ov5647_ctrls {
	struct video_ctrl auto_white_balance;
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl exposure_auto;
	struct video_ctrl exposure;
	struct video_ctrl auto_gain;
	struct video_ctrl analogue_gain;
	struct video_ctrl linkfreq;
	struct video_ctrl test_pattern;
};

struct ov5647_data {
	struct ov5647_ctrls ctrls;
	struct video_format fmt;
	uint16_t vts;
	int fps;
};

struct ov5647_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec powerdown_gpio;
	const int64_t *link_frequency;
	uint32_t pixel_rate_hz;
	bool clock_noncontinuous;
	bool horizontal_flip;
	bool vertical_flip;
};

/*
 * Base configuration of the sensor, at full resolution and ten bits per pixel, taken
 * from the vendor register table as is: it is the only geometry the vendor publishes
 * for this pixel format, and the crop window below is applied on top of it.
 *
 * The software standby and reset writes that surround the vendor table are left out,
 * since the driver performs the reset itself and starts the stream on request.
 */
static const struct video_reg16 ov5647_init_regs[] = {
	{0x3034, 0x1a},
	{0x3035, 0x21},
	{0x3036, 0x69},
	{0x303c, 0x11},
	{0x3106, 0xf5},
	{0x3821, 0x06},
	{0x3820, 0x00},
	{0x3827, 0xec},
	{0x370c, 0x03},
	{0x3612, 0x5b},
	{0x3618, 0x04},
	{0x5000, 0x06},
	{0x5002, 0x41},
	{0x5003, 0x08},
	{0x5a00, 0x08},
	/*
	 * The three system reset registers release every block, the MIPI
	 * transmitter among them. Leaving any of them set keeps that block in
	 * reset, which the sensor reports in no way at all: register access goes
	 * on working over I2C while the clock lane is never driven.
	 */
	{0x3000, 0x00},
	{0x3001, 0x00},
	{0x3002, 0x00},
	{0x3016, 0x08},
	{0x3017, 0xe0},
	{0x3018, 0x44},
	{0x301c, 0xf8},
	{0x301d, 0xf0},
	{0x3a18, 0x00},
	{0x3a19, 0xf8},
	{0x3c01, 0x80},
	{0x3b07, 0x0c},
	{0x380c, 0x0b},
	{0x380d, 0x1c},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3708, 0x64},
	{0x3709, 0x12},
	{0x3808, 0x0a},
	{0x3809, 0x20},
	{0x380a, 0x07},
	{0x380b, 0x98},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0xa3},
	{0x3811, 0x10},
	{0x3813, 0x06},
	{0x3630, 0x2e},
	{0x3632, 0xe2},
	{0x3633, 0x23},
	{0x3634, 0x44},
	{0x3636, 0x06},
	{0x3620, 0x64},
	{0x3621, 0xe0},
	{0x3600, 0x37},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x3731, 0x02},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3f05, 0x02},
	{0x3f06, 0x10},
	{0x3f01, 0x0a},
	{0x3a08, 0x01},
	{0x3a09, 0x28},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0d, 0x08},
	{0x3a0e, 0x06},
	{0x3a0f, 0x58},
	{0x3a10, 0x50},
	{0x3a1b, 0x58},
	{0x3a1e, 0x50},
	{0x3a11, 0x60},
	{0x3a1f, 0x28},
	{0x4001, 0x02},
	{0x4004, 0x04},
	{0x4000, 0x09},
	{0x4837, 0x19},
	{0x4800, 0x24},
	{0x3503, 0x03},
};

/*
 * Number of lines per frame, vertical blanking included, that achieves the given
 * frame rate at the fixed line length and pixel rate of the base configuration. It
 * is never shorter than the frame itself plus the minimum blanking, so the taller
 * resolutions run slower than requested.
 */
static uint16_t ov5647_frm_length(const struct device *dev, uint32_t height, uint32_t fps)
{
	const struct ov5647_config *cfg = dev->config;
	uint32_t lines = cfg->pixel_rate_hz / (OV5647_LINE_LENGTH * fps);

	return MIN(MAX(lines, height + OV5647_VBLANK_MIN), OV5647_VTS_MAX);
}

/* Frame rate that the frame length above actually achieves */
static uint32_t ov5647_frame_rate(const struct device *dev, uint32_t height, uint32_t fps)
{
	const struct ov5647_config *cfg = dev->config;

	return cfg->pixel_rate_hz / (OV5647_LINE_LENGTH * ov5647_frm_length(dev, height, fps));
}

/*
 * The exposure time cannot exceed the frame it is taken in, so its range follows the
 * frame length and the value in use is brought back within it.
 */
static int ov5647_update_exposure_range(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;

	ctrls->exposure.range.max = drv_data->vts - OV5647_EXPOSURE_MARGIN;

	if (ctrls->exposure.val <= ctrls->exposure.range.max) {
		return 0;
	}

	ctrls->exposure.val = ctrls->exposure.range.max;

	return video_write_cci_reg(&cfg->i2c, OV5647_CCI_EXPOSURE,
				   ctrls->exposure.val << OV5647_EXPOSURE_SHIFT);
}

/*
 * The Bayer order of the base configuration starts on a blue pixel, and the crop
 * window keeps it there: both resolution steps are multiples of four, so both crop
 * start addresses stay even and the phase of the pattern never shifts.
 *
 * TODO switch to video_set_selection() API for cropping instead
 */
static const struct video_format_cap ov5647_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_SBGGR10P,
		.width_min = 4, .width_max = OV5647_ACTIVE_WIDTH, .width_step = 4,
		.height_min = 4, .height_max = OV5647_ACTIVE_HEIGHT, .height_step = 4,
	},
	{0},
};

static int ov5647_set_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	uint32_t analog_width = fmt->width + OV5647_MARGIN_WIDTH;
	uint32_t analog_height = fmt->height + OV5647_MARGIN_HEIGHT;
	uint16_t vts = ov5647_frm_length(dev, fmt->height, drv_data->fps);
	struct video_reg crop_regs[] = {
		/* Area of the pixel array to read out, centered on it */
		{OV5647_CCI_X_ADDR_START, (OV5647_NATIVE_WIDTH - analog_width) / 2},
		{OV5647_CCI_X_ADDR_END, (OV5647_NATIVE_WIDTH + analog_width) / 2 - 1},
		{OV5647_CCI_Y_ADDR_START, (OV5647_NATIVE_HEIGHT - analog_height) / 2},
		{OV5647_CCI_Y_ADDR_END, (OV5647_NATIVE_HEIGHT + analog_height) / 2 - 1},

		/* Update the output resolution */
		{OV5647_CCI_X_OUTPUT_SIZE, fmt->width},
		{OV5647_CCI_Y_OUTPUT_SIZE, fmt->height},

		/* Frame length for the frame rate in use, at least the new output size */
		{OV5647_CCI_VTS, vts},
	};
	size_t idx;
	int ret;

	ret = video_format_caps_index(ov5647_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Format requested '%s' %ux%u not supported",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
		return -ENOTSUP;
	}

	if (fmt->width % ov5647_fmts[idx].width_step != 0) {
		LOG_ERR("Unsupported width %u requested", fmt->width);
		return -EINVAL;
	}

	if (fmt->height % ov5647_fmts[idx].height_step != 0) {
		LOG_ERR("Unsupported height %u requested", fmt->height);
		return -EINVAL;
	}

	ret = video_write_cci_multiregs(&cfg->i2c, crop_regs, ARRAY_SIZE(crop_regs));
	if (ret < 0) {
		return ret;
	}

	drv_data->fmt = *fmt;
	drv_data->vts = vts;

	return ov5647_update_exposure_range(dev);
}

static int ov5647_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct ov5647_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

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

enum {
	OV5647_30FPS_IDX,
	OV5647_15FPS_IDX,
};

enum {
	OV5647_30FPS = 30,
	OV5647_15FPS = 15,
};

static const uint32_t ov5647_framerates[] = {
	[OV5647_30FPS_IDX] = OV5647_30FPS,
	[OV5647_15FPS_IDX] = OV5647_15FPS,
};

static int ov5647_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	if (fie->index >= ARRAY_SIZE(ov5647_framerates)) {
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
	struct ov5647_data *drv_data = dev->data;
	struct video_frmival_enum fie = {
		.discrete = *frmival,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.format = &drv_data->fmt,
	};
	int ret;

	ret = video_closest_frmival(dev, &fie);
	if (ret < 0) {
		return ret;
	}

	drv_data->fps = ov5647_framerates[fie.index];
	drv_data->vts = ov5647_frm_length(dev, drv_data->fmt.height, drv_data->fps);

	ret = video_write_cci_reg(&cfg->i2c, OV5647_CCI_VTS, drv_data->vts);
	if (ret < 0) {
		return ret;
	}

	frmival->numerator = 1;
	frmival->denominator = ov5647_frame_rate(dev, drv_data->fmt.height, drv_data->fps);

	return ov5647_update_exposure_range(dev);
}

static int ov5647_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ov5647_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = ov5647_frame_rate(dev, drv_data->fmt.height, drv_data->fps);

	return 0;
}

/*
 * Hold the lanes in low power state while the stream is off, so that the receiver
 * sees them settle into LP-11 before the first frame.
 */
/*
 * Tag the outgoing packets with virtual channel 0. The register sequence above
 * leaves the field alone, and a demux filtering on the channel drops every
 * packet of any other one without the receiver reporting an error, so the
 * channel has to be selected rather than assumed.
 */
static int ov5647_set_virtual_channel(const struct device *dev, uint8_t channel)
{
	const struct ov5647_config *cfg = dev->config;
	uint32_t reg;
	int ret;

	ret = video_read_cci_reg(&cfg->i2c, OV5647_CCI_MIPI_CTRL14, &reg);
	if (ret < 0) {
		return ret;
	}

	reg &= ~OV5647_MIPI_CTRL14_VC;
	reg |= FIELD_PREP(OV5647_MIPI_CTRL14_VC, channel);

	return video_write_cci_reg(&cfg->i2c, OV5647_CCI_MIPI_CTRL14, reg);
}

static int ov5647_set_lanes_off(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct video_reg regs[] = {
		{OV5647_CCI_MIPI_CTRL00, OV5647_MIPI_CTRL00_CLOCK_LANE_GATE |
					 OV5647_MIPI_CTRL00_BUS_IDLE |
					 OV5647_MIPI_CTRL00_CLOCK_LANE_DISABLE},
		{OV5647_CCI_FRAME_OFF_NUMBER, 0x0f},
		{OV5647_CCI_PAD_OUT, 0x01},
	};

	return video_write_cci_multiregs(&cfg->i2c, regs, ARRAY_SIZE(regs));
}

static int ov5647_set_lanes_on(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	uint32_t ctrl00 = OV5647_MIPI_CTRL00_BUS_IDLE;
	struct video_reg regs[3];

	/*
	 * Gating the clock lane between packets only works if the receiver tracks it,
	 * so it is left running unless the devicetree says otherwise.
	 */
	if (cfg->clock_noncontinuous) {
		ctrl00 |= OV5647_MIPI_CTRL00_CLOCK_LANE_GATE |
			  OV5647_MIPI_CTRL00_LINE_SYNC_ENABLE;
	}

	regs[0] = (struct video_reg){OV5647_CCI_MIPI_CTRL00, ctrl00};
	regs[1] = (struct video_reg){OV5647_CCI_FRAME_OFF_NUMBER, 0x00};
	regs[2] = (struct video_reg){OV5647_CCI_PAD_OUT, 0x00};

	return video_write_cci_multiregs(&cfg->i2c, regs, ARRAY_SIZE(regs));
}

static int ov5647_set_stream(const struct device *dev, bool on, enum video_buf_type type)
{
	const struct ov5647_config *cfg = dev->config;
	int ret;

	if (type != VIDEO_BUF_TYPE_OUTPUT) {
		LOG_ERR("Only output buffers supported");
		return -EINVAL;
	}

	if (!on) {
		ret = video_write_cci_reg(&cfg->i2c, OV5647_CCI_SW_STANDBY,
					  OV5647_SW_STANDBY_STANDBY);
		if (ret < 0) {
			return ret;
		}

		return ov5647_set_lanes_off(dev);
	}

	ret = video_write_cci_reg(&cfg->i2c, OV5647_CCI_SW_STANDBY, OV5647_SW_STANDBY_STREAMING);
	if (ret < 0) {
		return ret;
	}

	return ov5647_set_lanes_on(dev);
}

/*
 * The exposure time and the analog gain are only the driver's own as long as the
 * sensor is not driving them itself, so they are read back from it and reported as
 * inactive while its automatic control is on.
 */
static void ov5647_update_exposure_flags(const struct device *dev)
{
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;

	if (ctrls->exposure_auto.val == VIDEO_EXPOSURE_MANUAL) {
		ctrls->exposure.flags &= ~(VIDEO_CTRL_FLAG_INACTIVE | VIDEO_CTRL_FLAG_VOLATILE);
	} else {
		ctrls->exposure.flags |= VIDEO_CTRL_FLAG_INACTIVE | VIDEO_CTRL_FLAG_VOLATILE;
	}
}

static int ov5647_set_aec_agc(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;
	uint32_t val = 0;

	if (ctrls->exposure_auto.val == VIDEO_EXPOSURE_MANUAL) {
		val |= OV5647_AEC_MANUAL;
	}

	if (ctrls->auto_gain.val == 0) {
		val |= OV5647_AGC_MANUAL;
	}

	return video_write_cci_reg(&cfg->i2c, OV5647_CCI_AEC_AGC, val);
}

static const uint8_t ov5647_test_pattern_val[] = {0x00, 0x80, 0x82, 0x81};

/*
 * Each axis has a pair of bits, one flipping the readout of the pixel array and one
 * flipping the window the ISP takes out of it. Only the pair together leaves the
 * Bayer phase of the first output row where it was: either bit alone shifts the
 * pattern by a row or a column, which swaps red for blue in whatever interpolates
 * the frames further down the pipeline.
 *
 * The register sequence leaves the vertical pair clear and the horizontal pair set,
 * so that zero means "as the sequence leaves it" on both axes, at the price of the
 * horizontal sense reading inverted here.
 */
static int ov5647_set_hflip(const struct device *dev, bool flip)
{
	const struct ov5647_config *cfg = dev->config;

	return video_modify_cci_reg(&cfg->i2c, OV5647_CCI_TIMING_TC_REG21,
				    OV5647_TIMING_TC_FLIP, flip ? 0 : OV5647_TIMING_TC_FLIP);
}

static int ov5647_set_vflip(const struct device *dev, bool flip)
{
	const struct ov5647_config *cfg = dev->config;

	return video_modify_cci_reg(&cfg->i2c, OV5647_CCI_TIMING_TC_REG20,
				    OV5647_TIMING_TC_FLIP, flip ? OV5647_TIMING_TC_FLIP : 0);
}

static int ov5647_set_ctrl(const struct device *dev, unsigned int cid)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	switch (cid) {
	case VIDEO_CID_EXPOSURE_AUTO:
		ret = ov5647_set_aec_agc(dev);
		if (ret < 0) {
			return ret;
		}

		ov5647_update_exposure_flags(dev);

		return 0;
	case VIDEO_CID_EXPOSURE:
		return video_write_cci_reg(&cfg->i2c, OV5647_CCI_EXPOSURE,
					   ctrls->exposure.val << OV5647_EXPOSURE_SHIFT);
	case VIDEO_CID_AUTOGAIN:
		ret = ov5647_set_aec_agc(dev);
		if (ret < 0) {
			return ret;
		}

		if (ctrls->auto_gain.val != 0) {
			return 0;
		}

		__fallthrough;
	case VIDEO_CID_ANALOGUE_GAIN:
		return video_write_cci_reg(&cfg->i2c, OV5647_CCI_GAIN, ctrls->analogue_gain.val);
	case VIDEO_CID_AUTO_WHITE_BALANCE:
		return video_write_cci_reg(&cfg->i2c, OV5647_CCI_AWB,
					   ctrls->auto_white_balance.val ? OV5647_AWB_ENABLE : 0);
	case VIDEO_CID_HFLIP:
		return ov5647_set_hflip(dev, ctrls->hflip.val != 0);
	case VIDEO_CID_VFLIP:
		return ov5647_set_vflip(dev, ctrls->vflip.val != 0);
	case VIDEO_CID_TEST_PATTERN:
		return video_write_cci_reg(&cfg->i2c, OV5647_CCI_ISPCTRL3D,
					   ov5647_test_pattern_val[ctrls->test_pattern.val]);
	default:
		LOG_WRN("Control not supported");
		return -ENOTSUP;
	}
}

static int ov5647_get_volatile_ctrl(const struct device *dev, uint32_t cid)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;
	uint32_t reg;
	int ret;

	switch (cid) {
	case VIDEO_CID_AUTOGAIN:
		ret = video_read_cci_reg(&cfg->i2c, OV5647_CCI_GAIN, &reg);
		if (ret < 0) {
			return ret;
		}

		ctrls->analogue_gain.val = CLAMP(reg, OV5647_GAIN_MIN, OV5647_GAIN_MAX);

		return 0;
	case VIDEO_CID_EXPOSURE:
		ret = video_read_cci_reg(&cfg->i2c, OV5647_CCI_EXPOSURE, &reg);
		if (ret < 0) {
			return ret;
		}

		ctrls->exposure.val = CLAMP(reg >> OV5647_EXPOSURE_SHIFT,
					    ctrls->exposure.range.min,
					    ctrls->exposure.range.max);

		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(video, ov5647_driver_api) = {
	.set_stream = ov5647_set_stream,
	.set_ctrl = ov5647_set_ctrl,
	.get_volatile_ctrl = ov5647_get_volatile_ctrl,
	.set_format = ov5647_set_fmt,
	.get_format = ov5647_get_fmt,
	.get_caps = ov5647_get_caps,
	.set_frmival = ov5647_set_frmival,
	.get_frmival = ov5647_get_frmival,
	.enum_frmival = ov5647_enum_frmival,
};

/* Only the two modes the sensor implements, named as the standard menu names them */
static const char *const ov5647_exposure_auto_menu[] = {
	"Auto Mode",
	"Manual Mode",
	NULL,
};

static const char *const ov5647_test_pattern_menu[] = {
	"Disabled",
	"Color Bars",
	"Color Squares",
	"Random Data",
	NULL,
};

static int ov5647_init_ctrls(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct ov5647_data *drv_data = dev->data;
	struct ov5647_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	ret = video_init_ctrl(&ctrls->auto_white_balance, dev, VIDEO_CID_AUTO_WHITE_BALANCE,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1,
							.def = cfg->horizontal_flip});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1,
							.def = cfg->vertical_flip});
	if (ret < 0) {
		return ret;
	}

	/*
	 * The automatic exposure is not clustered over the manual one, unlike the gain
	 * below: the framework takes a primary control value of zero to mean manual,
	 * and zero is precisely what selects the automatic mode of this one. The two
	 * flags a cluster maintains are handled directly instead.
	 */
	ret = video_init_menu_ctrl(&ctrls->exposure_auto, dev, VIDEO_CID_EXPOSURE_AUTO,
				   VIDEO_EXPOSURE_AUTO, ov5647_exposure_auto_menu);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
		(struct video_ctrl_range){.min = OV5647_EXPOSURE_MIN,
					  .max = drv_data->vts - OV5647_EXPOSURE_MARGIN,
					  .step = 1,
					  .def = MIN(OV5647_EXPOSURE_DEFAULT,
						     drv_data->vts - OV5647_EXPOSURE_MARGIN)});
	if (ret < 0) {
		return ret;
	}

	ov5647_update_exposure_flags(dev);

	ret = video_init_ctrl(&ctrls->auto_gain, dev, VIDEO_CID_AUTOGAIN,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 1});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(&ctrls->analogue_gain, dev, VIDEO_CID_ANALOGUE_GAIN,
			      (struct video_ctrl_range){.min = OV5647_GAIN_MIN,
							.max = OV5647_GAIN_MAX,
							.step = 1,
							.def = OV5647_GAIN_DEFAULT});
	if (ret < 0) {
		return ret;
	}

	ret = video_auto_cluster_ctrl(&ctrls->auto_gain, 2, true);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(&ctrls->linkfreq, dev, VIDEO_CID_LINK_FREQ, 0,
				       cfg->link_frequency, 1);
	if (ret < 0) {
		return ret;
	}

	ctrls->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev, VIDEO_CID_TEST_PATTERN, 0,
				   ov5647_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The register table leaves both automatic controls off, so the sensor is told
	 * about the defaults above, which turn them on.
	 */
	return ov5647_set_aec_agc(dev);
}

static int ov5647_power_up(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	int ret;

	if (cfg->powerdown_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->powerdown_gpio)) {
			LOG_ERR("GPIO device %s is not ready", cfg->powerdown_gpio.port->name);
			return -ENODEV;
		}

		/* Assert power down, then release it to bring the sensor up from a known state */
		ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(5));

		ret = gpio_pin_set_dt(&cfg->powerdown_gpio, 0);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("GPIO device %s is not ready", cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(5));

		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(K_MSEC(OV5647_PWDN_DELAY_MS));

	return 0;
}

static int ov5647_init(const struct device *dev)
{
	const struct ov5647_config *cfg = dev->config;
	struct video_format fmt = {
		.width = ov5647_fmts[0].width_min,
		.height = ov5647_fmts[0].height_min,
		.pixelformat = ov5647_fmts[0].pixelformat,
	};
	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = OV5647_30FPS,
	};
	uint32_t reg;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = ov5647_power_up(dev);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, OV5647_CCI_SW_RESET, 1);
	if (ret < 0) {
		return ret;
	}

	k_sleep(K_MSEC(5));

	ret = video_read_cci_reg(&cfg->i2c, OV5647_CCI_CHIP_ID, &reg);
	if (ret < 0) {
		return ret;
	}

	if (reg != OV5647_CHIP_ID) {
		LOG_ERR("Wrong chip ID 0x%04x instead of 0x%04x", reg, OV5647_CHIP_ID);
		return -ENODEV;
	}

	ret = video_write_cci_multiregs16(&cfg->i2c, ov5647_init_regs,
					  ARRAY_SIZE(ov5647_init_regs));
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_hflip(dev, cfg->horizontal_flip);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_vflip(dev, cfg->vertical_flip);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_virtual_channel(dev, 0);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_lanes_off(dev);
	if (ret < 0) {
		return ret;
	}

	/* The frame rate comes first, since the format derives its frame length from it */
	ret = ov5647_set_frmival(dev, &frmival);
	if (ret < 0) {
		return ret;
	}

	ret = ov5647_set_fmt(dev, &fmt);
	if (ret < 0) {
		return ret;
	}

	return ov5647_init_ctrls(dev);
}

#define OV5647_INIT(n)                                                                             \
	static struct ov5647_data ov5647_data_##n;                                                 \
                                                                                                   \
	static const int64_t ov5647_link_frequency_##n[] = {                                       \
		OV5647_LINK_FREQ_HZ(DT_INST_PROP_BY_PHANDLE(n, clocks, clock_frequency)),          \
	};                                                                                         \
                                                                                                   \
	static const struct ov5647_config ov5647_cfg_##n = {                                       \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
		.powerdown_gpio = GPIO_DT_SPEC_INST_GET_OR(n, powerdown_gpios, {0}),               \
		.link_frequency = ov5647_link_frequency_##n,                                       \
		.pixel_rate_hz =                                                                   \
			OV5647_PIXEL_RATE_HZ(DT_INST_PROP_BY_PHANDLE(n, clocks,                    \
								     clock_frequency)),            \
		.clock_noncontinuous = DT_INST_PROP(n, clock_noncontinuous),                       \
		.horizontal_flip = DT_INST_PROP(n, horizontal_flip),                               \
		.vertical_flip = DT_INST_PROP(n, vertical_flip),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ov5647_init, NULL, &ov5647_data_##n, &ov5647_cfg_##n,            \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov5647_driver_api);        \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(ov5647_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(OV5647_INIT)
