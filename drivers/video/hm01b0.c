/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hm01b0

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(hm01b0, CONFIG_VIDEO_LOG_LEVEL);
#define MAX_FRAME_RATE 10
#define MIN_FRAME_RATE 1
#define HM01B0_ID      0x01B0
#define CHECK_CONNECTION_DELAY_MS 1
#define HM01B0_LINE_LEN_PCLK_IDX 5

#define HM01B0_REG8(addr)             ((addr) | VIDEO_REG_ADDR16_DATA8)
#define HM01B0_REG16(addr)            ((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define HM01B0_CCI_ID                 HM01B0_REG16(0x0000)
#define HM01B0_CCI_STS                HM01B0_REG8(0x0100)
#define HM01B0_CCI_IMG_ORIENTATION    HM01B0_REG8(0x0101)
#define HM01B0_CCI_RESET              HM01B0_REG8(0x0103)
#define HM01B0_CCI_GRP_PARAM_HOLD     HM01B0_REG8(0x0104)
#define HM01B0_CCI_INTEGRATION_H      HM01B0_REG16(0x0202)
#define HM01B0_CCI_ANALOG_GAIN        HM01B0_REG8(0x0205)
#define HM01B0_CCI_DIGITAL_GAIN_H     HM01B0_REG16(0x020E)
#define HM01B0_CCI_FRAME_LENGTH_LINES HM01B0_REG16(0x0340)
#define HM01B0_CCI_LINE_LENGTH_PCLK   HM01B0_REG16(0x0342)
#define HM01B0_CCI_WIDTH              HM01B0_REG8(0x0383)
#define HM01B0_CCI_HEIGHT             HM01B0_REG8(0x0387)
#define HM01B0_CCI_BINNING_MODE       HM01B0_REG8(0x0390)
#define HM01B0_CCI_BLC_CFG            HM01B0_REG8(0x1000)
#define HM01B0_CCI_BLC_TGT            HM01B0_REG8(0x1003)
#define HM01B0_CCI_BLI_EN             HM01B0_REG8(0x1006)
#define HM01B0_CCI_BLC2_TGT           HM01B0_REG8(0x1007)
#define HM01B0_CCI_DPC_CTRL           HM01B0_REG8(0x1008)
#define HM01B0_CCI_SINGLE_THR_HOT     HM01B0_REG8(0x100B)
#define HM01B0_CCI_SINGLE_THR_COLD    HM01B0_REG8(0x100C)
#define HM01B0_CCI_STATISTIC_CTRL     HM01B0_REG8(0x2000)
#define HM01B0_CCI_AE_CTRL            HM01B0_REG8(0x2100)
#define HM01B0_CCI_AE_TARGET_MEAN     HM01B0_REG8(0x2101)
#define HM01B0_CCI_AE_MIN_MEAN        HM01B0_REG8(0x2102)
#define HM01B0_CCI_CONVERGE_IN_TH     HM01B0_REG8(0x2103)
#define HM01B0_CCI_CONVERGE_OUT_TH    HM01B0_REG8(0x2104)
#define HM01B0_CCI_MAX_INTG           HM01B0_REG16(0x2105)
#define HM01B0_CCI_MIN_INTG           HM01B0_REG8(0x2107)
#define HM01B0_CCI_MAX_AGAIN_FULL     HM01B0_REG8(0x2108)
#define HM01B0_CCI_MAX_AGAIN_BIN2     HM01B0_REG8(0x2109)
#define HM01B0_CCI_MIN_AGAIN          HM01B0_REG8(0x210A)
#define HM01B0_CCI_MAX_DGAIN          HM01B0_REG8(0x210B)
#define HM01B0_CCI_MIN_DGAIN          HM01B0_REG8(0x210C)
#define HM01B0_CCI_DAMPING_FACTOR     HM01B0_REG8(0x210D)
#define HM01B0_CCI_FS_CTRL            HM01B0_REG8(0x210E)
#define HM01B0_CCI_FS_60HZ_H          HM01B0_REG16(0x210F)
#define HM01B0_CCI_FS_50HZ_H          HM01B0_REG16(0x2111)
#define HM01B0_CCI_FS_HYST_TH         HM01B0_REG8(0x2113)
#define HM01B0_CCI_MD_CTRL            HM01B0_REG8(0x2150)
#define HM01B0_CCI_QVGA_WIN_EN        HM01B0_REG8(0x3010)
#define HM01B0_CCI_BIT_CONTROL        HM01B0_REG8(0x3059)
#define HM01B0_CCI_OSC_CLOCK_DIV      HM01B0_REG8(0x3060)
#define HM01B0_CCI_ANA_REGISTER_17    HM01B0_REG8(0x3067)

#define HM01B0_ORIENTATION_HMIRROR BIT(0)
#define HM01B0_ORIENTATION_VMIRROR BIT(1)
#define HM01B0_CCI_OSC_CLOCK_DIV_VT_REG_DIV_1 0x08 /* vt_reg_div[3:2]=10 */

#define HM01B0_CTRL_VAL(data_bits) \
	((data_bits) == 8 ? 0x02 : \
	(data_bits) == 4 ? 0x42 : \
	(data_bits) == 1 ? 0x22 : 0x00)

/* Note: Bayer versions do not support 160x120 type settings */
enum hm01b0_resolution {
	RESOLUTION_160x120,
	RESOLUTION_320x240,
	RESOLUTION_320x320,
	RESOLUTION_162x122,
	RESOLUTION_324x244,
	RESOLUTION_324x324,
	RESOLUTION_162x122_Y4,
	RESOLUTION_324x244_Y4,
	RESOLUTION_324x324_Y4,
	RESOLUTION_324x244_BAYER,
	RESOLUTION_324x324_BAYER,
};

enum {
	HM01B0_5_OR_8_FPS,
	HM01B0_11_OR_15_FPS,
	HM01B0_22_OR_30_FPS,
	HM01B0_45_OR_60_FPS,
};

#define HIMAX_LINE_LEN_PCK_FULL     0x178
#define HIMAX_FRAME_LENGTH_FULL     0x109

#define HIMAX_LINE_LEN_PCK_QVGA     0x178
#define HIMAX_FRAME_LENGTH_QVGA     0x104

#define HIMAX_LINE_LEN_PCK_QQVGA    0x178
#define HIMAX_FRAME_LENGTH_QQVGA    0x084

struct video_reg hm01b0_160x120_regs[] = {
	{HM01B0_CCI_WIDTH, 0x3},
	{HM01B0_CCI_HEIGHT, 0x3},
	{HM01B0_CCI_BINNING_MODE, 0x3},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_MAX_INTG, HIMAX_FRAME_LENGTH_QQVGA - 2},
	{HM01B0_CCI_FRAME_LENGTH_LINES, HIMAX_FRAME_LENGTH_QQVGA},
	{HM01B0_CCI_LINE_LENGTH_PCLK, HIMAX_LINE_LEN_PCK_QQVGA},
};

struct video_reg hm01b0_320x240_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_MAX_INTG, HIMAX_FRAME_LENGTH_QVGA - 2},
	{HM01B0_CCI_FRAME_LENGTH_LINES, HIMAX_FRAME_LENGTH_QVGA},
	{HM01B0_CCI_LINE_LENGTH_PCLK, HIMAX_LINE_LEN_PCK_QVGA},
};

struct video_reg hm01b0_320x320_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x0},
	{HM01B0_CCI_MAX_INTG, HIMAX_FRAME_LENGTH_QVGA - 2},
	{HM01B0_CCI_FRAME_LENGTH_LINES, HIMAX_FRAME_LENGTH_FULL},
	{HM01B0_CCI_LINE_LENGTH_PCLK, HIMAX_LINE_LEN_PCK_FULL},
};

struct video_reg *hm01b0_init_regs[] = {
	[RESOLUTION_160x120] = hm01b0_160x120_regs,
	[RESOLUTION_320x240] = hm01b0_320x240_regs,
	[RESOLUTION_320x320] = hm01b0_320x320_regs,
	[RESOLUTION_162x122] = hm01b0_160x120_regs,
	[RESOLUTION_324x244] = hm01b0_320x240_regs,
	[RESOLUTION_324x324] = hm01b0_320x320_regs,
	[RESOLUTION_162x122_Y4] = hm01b0_160x120_regs,
	[RESOLUTION_324x244_Y4] = hm01b0_320x240_regs,
	[RESOLUTION_324x324_Y4] = hm01b0_320x320_regs,
	[RESOLUTION_324x244_BAYER] = hm01b0_320x240_regs,
	[RESOLUTION_324x324_BAYER] = hm01b0_320x320_regs,
};

struct video_reg hm01b0_default_regs[] = {
	{HM01B0_CCI_BLC_TGT,		0x08},	/* BLC target :8  at 8 bit mode */
	{HM01B0_CCI_BLC2_TGT,		0x08},	/* BLI target :8  at 8 bit mode */
	{HM01B0_REG8(0x3044),		0x0A},	/* Increase CDS time for settling */
	{HM01B0_REG8(0x3045),		0x00},	/* Make symmetric for cds_tg and rst_tg */
	{HM01B0_REG8(0x3047),		0x0A},	/* Increase CDS time for settling */
	{HM01B0_REG8(0x3050),		0xC0},  /* Make negative offset up to 4x */
	{HM01B0_REG8(0x3051),		0x42},
	{HM01B0_REG8(0x3052),		0x50},
	{HM01B0_REG8(0x3053),		0x00},
	{HM01B0_REG8(0x3054),		0x03},	/* tuning sf sig clamping as lowest */
	{HM01B0_REG8(0x3055),		0xF7},	/* tuning dsun */
	{HM01B0_REG8(0x3056),		0xF8},	/* increase adc nonoverlap clk */
	{HM01B0_REG8(0x3057),		0x29},	/* increase adc pwr for missing code */
	{HM01B0_REG8(0x3058),		0x1F},	/* turn on dsun */
	{HM01B0_REG8(0x3059),		0x1E},
	{HM01B0_REG8(0x3064),		0x00},
	{HM01B0_REG8(0x3065),		0x04},	/* pad pull 0 */
	{HM01B0_CCI_ANA_REGISTER_17,	0x00},	/* Disable internal oscillator */

	{HM01B0_CCI_BLC_CFG,		0x43},	/* BLC_on, IIR */

	{HM01B0_REG8(0x1001),		0x43},	/* BLC dithering en */
	{HM01B0_REG8(0x1002),		0x43},	/* blc_darkpixel_thd */
	{HM01B0_REG8(0x0350),		0x7F},	/* Dgain Control */
	{HM01B0_CCI_BLI_EN,		0x01},	/* BLI enable */
	{HM01B0_REG8(0x1003),		0x00},	/* BLI Target [Def: 0x20] */

	{HM01B0_CCI_DPC_CTRL,           0x01},	/* DPC option 0:DPC off 1:mono3:bayer1 5:bayer2 */
	{HM01B0_REG8(0x1009),           0xA0},	/* cluster hot pixel th */
	{HM01B0_REG8(0x100A),           0x60},	/* cluster cold pixel th */
	{HM01B0_CCI_SINGLE_THR_HOT,     0x90},	/* single hot pixel th */
	{HM01B0_CCI_SINGLE_THR_COLD,    0x40},	/* single cold pixel th */
	{HM01B0_REG8(0x1012),           0x00},	/* Sync. shift disable */
	{HM01B0_CCI_STATISTIC_CTRL,     0x07},	/* AE stat en | MD LROI stat en | magic */
	{HM01B0_REG8(0x2003),           0x00},
	{HM01B0_REG8(0x2004),           0x1C},
	{HM01B0_REG8(0x2007),           0x00},
	{HM01B0_REG8(0x2008),           0x58},
	{HM01B0_REG8(0x200B),		0x00},
	{HM01B0_REG8(0x200C),		0x7A},
	{HM01B0_REG8(0x200F),		0x00},
	{HM01B0_REG8(0x2010),		0xB8},
	{HM01B0_REG8(0x2013),		0x00},
	{HM01B0_REG8(0x2014),		0x58},
	{HM01B0_REG8(0x2017),		0x00},
	{HM01B0_REG8(0x2018),		0x9B},

	{HM01B0_CCI_AE_CTRL,		0x01},	/* Automatic Exposure */
	{HM01B0_CCI_AE_TARGET_MEAN,	0x64},	/* AE target mean          [Def: 0x3C] */
	{HM01B0_CCI_AE_MIN_MEAN,	0x0A},	/* AE min target mean      [Def: 0x0A] */
	{HM01B0_CCI_CONVERGE_IN_TH,	0x03},	/* Converge in threshold   [Def: 0x03] */
	{HM01B0_CCI_CONVERGE_OUT_TH,	0x05},	/* Converge out threshold  [Def: 0x05] */
	{HM01B0_CCI_MAX_INTG,		(HIMAX_FRAME_LENGTH_QVGA - 2)},	/* Max INTG High Byte  */
	{HM01B0_CCI_MAX_AGAIN_FULL,	0x04},	/* Max Analog gain full frame mode [Def: 0x03] */
	{HM01B0_CCI_MAX_AGAIN_BIN2,	0x04},	/* Max Analog gain bin2 mode       [Def: 0x04] */
	{HM01B0_CCI_MAX_DGAIN,		0xC0},

	{HM01B0_CCI_INTEGRATION_H,	0x0108},	/* Integration H           [Def: 0x01] */
	{HM01B0_CCI_ANALOG_GAIN,	0x00},	/* Analog Global Gain      [Def: 0x00] */
	{HM01B0_CCI_DAMPING_FACTOR,	0x20},	/* Damping Factor          [Def: 0x20] */
	{HM01B0_CCI_DIGITAL_GAIN_H,	0x0100},	/* Digital Gain High       [Def: 0x01] */

	{HM01B0_CCI_FS_CTRL,		0x00},	/* Flicker Control */

	{HM01B0_CCI_FS_60HZ_H,		0x003C},
	{HM01B0_CCI_FS_50HZ_H,		0x0032},

	{HM01B0_CCI_MD_CTRL,		0x00},
	{HM01B0_CCI_FRAME_LENGTH_LINES, HIMAX_FRAME_LENGTH_QVGA},
	{HM01B0_CCI_LINE_LENGTH_PCLK,   HIMAX_LINE_LEN_PCK_QVGA},
	{HM01B0_CCI_QVGA_WIN_EN,	0x01},	/* Enable QVGA window readout */
	{HM01B0_REG8(0x0383),		0x01},
	{HM01B0_REG8(0x0387),		0x01},
	{HM01B0_REG8(0x0390),		0x00},
	{HM01B0_REG8(0x3011),		0x70},
	{HM01B0_REG8(0x3059),		0x02},
	{HM01B0_CCI_OSC_CLOCK_DIV,	0x0A},	/* vt_sys_div / 2 */
	{HM01B0_CCI_IMG_ORIENTATION,	0x00},	/* change the orientation */
	{HM01B0_REG8(0x0104),		0x01},
};

struct hm01b0_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct hm01b0_data {
	struct hm01b0_ctrls ctrls;
	struct video_format fmt;
};

struct hm01b0_config {
	const struct i2c_dt_spec i2c;
	const uint8_t ctrl_val;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

#define HM01B0_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}

static const struct video_format_cap hm01b0_fmts[] = {
	[RESOLUTION_160x120] = HM01B0_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_320x240] = HM01B0_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_320x320] = HM01B0_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_162x122] = HM01B0_VIDEO_FORMAT_CAP(162, 122, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_324x244] = HM01B0_VIDEO_FORMAT_CAP(324, 244, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_324x324] = HM01B0_VIDEO_FORMAT_CAP(324, 324, VIDEO_PIX_FMT_GREY),
	[RESOLUTION_162x122_Y4] = HM01B0_VIDEO_FORMAT_CAP(162, 122, VIDEO_PIX_FMT_Y4),
	[RESOLUTION_324x244_Y4] = HM01B0_VIDEO_FORMAT_CAP(324, 244, VIDEO_PIX_FMT_Y4),
	[RESOLUTION_324x324_Y4] = HM01B0_VIDEO_FORMAT_CAP(324, 324, VIDEO_PIX_FMT_Y4),
	[RESOLUTION_324x244_BAYER] = HM01B0_VIDEO_FORMAT_CAP(324, 244, VIDEO_PIX_FMT_SBGGR8),
	[RESOLUTION_324x324_BAYER] = HM01B0_VIDEO_FORMAT_CAP(324, 324, VIDEO_PIX_FMT_SBGGR8),
	{0},
};

static int hm01b0_apply_configuration(const struct device *dev, enum hm01b0_resolution resolution)
{
	const struct hm01b0_config *config = dev->config;
	int ret;

	/* Number of registers is the same for all configuration */
	ret = video_write_cci_multiregs(&config->i2c, hm01b0_init_regs[resolution],
					ARRAY_SIZE(hm01b0_160x120_regs));
	if (ret < 0) {
		LOG_ERR("Failed to write config list registers (%d)", ret);
		return ret;
	}

	/* REG_BIT_CONTROL */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_BIT_CONTROL, config->ctrl_val);
	if (ret < 0) {
		LOG_ERR("Failed to write BIT_CONTROL reg (%d)", ret);
		return ret;
	}

	/* INTEGRATION_H */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_INTEGRATION_H,
				  hm01b0_init_regs[resolution][HM01B0_LINE_LEN_PCLK_IDX].data / 2);
	if (ret < 0) {
		LOG_ERR("Failed to write INTEGRATION_H reg (%d)", ret);
		return ret;
	}

	/* GRP_PARAM_HOLD */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write GRP_PARAM_HOLD reg (%d)", ret);
		return ret;
	}

	return ret;
}

static int hm01b0_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = hm01b0_fmts;

	return 0;
}

static int hm01b0_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;
	size_t idx;
	int ret;

	LOG_DBG("HM01B0 set_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	ret = video_format_caps_index(hm01b0_fmts, fmt, &idx);
	if (ret != 0) {
		LOG_ERR("Image resolution not supported\n");
		return ret;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		return 0;
	}

	/* Check if camera is capable of handling given format */
	ret = hm01b0_apply_configuration(dev, (enum hm01b0_resolution)idx);
	if (ret != 0) {
		/* Camera is not capable of handling given format */
		LOG_ERR("Image resolution not supported");
		return ret;
	}
	data->fmt = *fmt;

	return ret;
}

static int hm01b0_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;

	*fmt = data->fmt;
	LOG_DBG("HM01B0 get_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	return 0;
}

static int hm01b0_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct hm01b0_config *config = dev->config;

	/* SET MODE_SELECT */
	return video_write_cci_reg(&config->i2c, HM01B0_CCI_STS, enable ? 1 : 0);
}

static int hm01b0_soft_reset(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t val = 0xff;
	int ret;

	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Error writing HM01B0_CCI_RESET (%d)", ret);
		return ret;
	}

	for (int retries = 0; retries < 10; retries++) {
		ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_STS, &val);
		if (ret != 0 || val == 0x0) {
			break;
		}
		k_msleep(100);
	}
	if (ret != 0) {
		LOG_ERR("Soft reset error (%d)", ret);
	}

	return ret;
}

static int hm01b0_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct hm01b0_config *config = dev->config;
	struct hm01b0_data *drv_data = dev->data;
	int ret;
	uint32_t osc_div = 0;
	struct video_frmival_enum fie;

	fie.format = &drv_data->fmt;
	fie.discrete = *frmival;
	fie.type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	video_closest_frmival(dev, &fie);

	/*
	 * Note: in highres mode max FPS is 45 else 60
	 *  OSC clock divider: 00=/8, 01=/4, 10=/2, 11=/1
	 *  The return index will be the same value that
	 *  we use for the osc_div field of OSC_CLOCK_DIV
	 */
	osc_div = fie.index;
	LOG_DBG("%u %u %u", fie.index, fie.discrete.numerator, fie.discrete.denominator);

	/* We also set the VT_REG_DIV bits[3:2] to 10 (0x2) or /1 */
	osc_div |= HM01B0_CCI_OSC_CLOCK_DIV_VT_REG_DIV_1;

	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_OSC_CLOCK_DIV, osc_div);
	if (ret < 0) {
		LOG_ERR("Failed to write OSC_CLK_DIV = %x reg (%d)", osc_div, ret);
		return ret;
	}

	/* GRP_PARAM_HOLD */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write GRP_PARAM_HOLD reg (%d)", ret);
		return ret;
	}

	LOG_DBG("FrameRate selected: %d %d = %d", fie.discrete.numerator, fie.discrete.denominator,
		fie.discrete.denominator / fie.discrete.numerator);
	LOG_DBG("OSC DIV: %d", osc_div);

	return 0;
}

static int hm01b0_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct hm01b0_config *config = dev->config;
	struct hm01b0_data *drv_data = dev->data;
	uint32_t reg;
	int ret;

	/* lets compute the frmival from the HM01B0_CCI_OSC_CLOCK_DIV registervalue */
	ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_OSC_CLOCK_DIV, &reg);
	if (ret < 0) {
		LOG_ERR("Failed to write OSC_CLK_DIV reg (%d)", ret);
		return ret;
	}

	frmival->denominator = ((drv_data->fmt.width >= 320)
		&& (drv_data->fmt.height >= 320)) ? 45 : 60;

	/* 0=>8 1->4 2->2 3->1 */
	frmival->numerator = 8 >> (reg & 0x3);

	return 0;
}

int hm01b0_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	struct hm01b0_data *drv_data = dev->data;

	fie->discrete.denominator = ((drv_data->fmt.width >= 320)
		&& (drv_data->fmt.height >= 320)) ? 45 : 60;

	switch (fie->index) {
	case HM01B0_5_OR_8_FPS:
		fie->discrete.numerator = 8;
		break;
	case HM01B0_11_OR_15_FPS:
		fie->discrete.numerator = 4;
		break;
	case HM01B0_22_OR_30_FPS:
		fie->discrete.numerator = 2;
		break;
	case HM01B0_45_OR_60_FPS:
		/* if we are doing 4 bit mode don't allow highest speed */
		if (fie->format && (fie->format->pixelformat == VIDEO_PIX_FMT_Y4)) {
			return -EINVAL;
		}
		fie->discrete.numerator = 1;
		break;
	default:
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	LOG_DBG("i:%u %u %u", fie->index, fie->discrete.numerator, fie->discrete.denominator);

	return 0;
}

static int hm01b0_init_controls(const struct device *dev)
{
	int ret;
	struct hm01b0_data *drv_data = dev->data;
	struct hm01b0_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int hm01b0_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct hm01b0_config *config = dev->config;
	struct hm01b0_data *drv_data = dev->data;
	struct hm01b0_ctrls *ctrls = &drv_data->ctrls;
	int ret;

	switch (id) {
	case VIDEO_CID_HFLIP:
		ret = video_modify_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION,
					   HM01B0_ORIENTATION_HMIRROR,
					   FIELD_PREP(HM01B0_ORIENTATION_HMIRROR,
						      ctrls->hflip.val));
		if (ret < 0) {
			return ret;
		}
		break;
	case VIDEO_CID_VFLIP:
		ret = video_modify_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION,
					   HM01B0_ORIENTATION_VMIRROR,
					   FIELD_PREP(HM01B0_ORIENTATION_VMIRROR,
						      ctrls->vflip.val));
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* if we get here it implies that register modified so tell system to use it */
	return video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
}

static DEVICE_API(video, hm01b0_driver_api) = {
	.set_format = hm01b0_set_fmt,
	.get_format = hm01b0_get_fmt,
	.set_ctrl = hm01b0_set_ctrl,
	.set_stream = hm01b0_set_stream,
	.get_caps = hm01b0_get_caps,
	.set_frmival = hm01b0_set_frmival,
	.get_frmival = hm01b0_get_frmival,
	.enum_frmival = hm01b0_enum_frmival,
};

static bool hm01b0_check_connection(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t model_id;
	int ret;

	ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_ID, &model_id);
	if (ret < 0) {
		LOG_ERR("Error reading id reg (%d)", ret);
		return false;
	}

	return (model_id == HM01B0_ID);
}

static int hm01b0_init(const struct device *dev)
{
	const struct hm01b0_config __maybe_unused *config = dev->config;
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios) || DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Reset camera module */
	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
	}
#endif
	/* give time for the camera to startup before checking it */
	k_msleep(CHECK_CONNECTION_DELAY_MS);
#endif

	if (!hm01b0_check_connection(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	ret = hm01b0_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("error soft reset (%d)", ret);
		return ret;
	}

	/* Try to reset the registers to the same as Arduino did at reset */
	ret = video_write_cci_multiregs(&config->i2c, hm01b0_default_regs,
					ARRAY_SIZE(hm01b0_default_regs));
	if (ret < 0) {
		LOG_ERR("Failed to write config list registers (%d)", ret);
		return ret;
	}

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width = 160,
		.height = 120,
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};

	ret = hm01b0_set_fmt(dev, &fmt);
	if (ret != 0) {
		LOG_ERR("Error setting video format (%d)", ret);
		return ret;
	}

	/* Initialize controls */
	return hm01b0_init_controls(dev);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define HM01B0_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define HM01B0_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define HM01B0_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define HM01B0_PWDN_GPIO(inst)
#endif

#define HM01B0_INIT(inst)                                                                       \
	const struct hm01b0_config hm01b0_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                              \
		.ctrl_val = HM01B0_CTRL_VAL(DT_INST_PROP(inst, data_bits)),                     \
		HM01B0_RESET_GPIO(inst)								\
		HM01B0_PWDN_GPIO(inst)								\
	};                                                                                      \
	struct hm01b0_data hm01b0_data_##inst;                                                  \
	DEVICE_DT_INST_DEFINE(inst, &hm01b0_init, NULL, &hm01b0_data_##inst,                    \
			      &hm01b0_config_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,   \
			      &hm01b0_driver_api);                                              \
	VIDEO_DEVICE_DEFINE(hm01b0_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(HM01B0_INIT)
