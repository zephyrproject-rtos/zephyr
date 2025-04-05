/*
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

LOG_MODULE_REGISTER(imx219, CONFIG_VIDEO_LOG_LEVEL);

#define IMX219_FULL_WIDTH		3280
#define IMX219_FULL_HEIGHT		2464

#define IMX219_REG8(addr)		((addr) | VIDEO_REG_ADDR16_DATA8)
#define IMX219_REG16(addr)		((addr) | VIDEO_REG_ADDR16_DATA16_BE)

#define IMX219_REG_CHIP_ID		IMX219_REG16(0x0000)
#define IMX219_REG_SOFTWARE_RESET	IMX219_REG8(0x0103)
#define IMX219_REG_MODE_SELECT		IMX219_REG8(0x0100)
#define IMX219_REG_ANALOG_GAIN		IMX219_REG8(0x0157)
#define IMX219_REG_DIGITAL_GAIN		IMX219_REG16(0x0158)
#define IMX219_REG_INTEGRATION_TIME	IMX219_REG16(0x015A)
#define IMX219_REG_TESTPATTERN		IMX219_REG16(0x0600)
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
#define IMX219_REG_CSI_DATA_FORMAT_A	IMX219_REG16(0x018c)
#define IMX219_REG_BINNING_MODE_H	IMX219_REG8(0x0174)
#define IMX219_REG_BINNING_MODE_V	IMX219_REG8(0x0175)
#define IMX219_REG_ORIENTATION		IMX219_REG8(0x0172)
#define IMX219_REG_FRM_LENGTH_A		IMX219_REG16(0x0160)
#define IMX219_REG_FRM_LENGTH_A		IMX219_REG16(0x0160)
#define IMX219_REG_X_ADD_STA_A		IMX219_REG16(0x0164)
#define IMX219_REG_X_ADD_END_A		IMX219_REG16(0x0166)
#define IMX219_REG_Y_ADD_STA_A		IMX219_REG16(0x0168)
#define IMX219_REG_Y_ADD_END_A		IMX219_REG16(0x016a)
#define IMX219_REG_X_OUTPUT_SIZE	IMX219_REG16(0x016c)
#define IMX219_REG_Y_OUTPUT_SIZE	IMX219_REG16(0x016e)
#define IMX219_REG_X_ODD_INC_A		IMX219_REG8(0x0170)
#define IMX219_REG_Y_ODD_INC_A		IMX219_REG8(0x0171)

/* Registers to crop down a resolution to a centered width and height */
static const struct video_reg init_regs[] = {
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
	{IMX219_REG_PREPLLCK_VT_DIV, 0x03},	/* Auto */
	{IMX219_REG_PREPLLCK_OP_DIV, 0x03},	/* Auto */
	{IMX219_REG_OPPXCK_DIV, 10},		/* 10-bits per pixel */
	{IMX219_REG_VTPXCK_DIV, 5},
	{IMX219_REG_VTSYCK_DIV, 1},
	{IMX219_REG_OPSYCK_DIV, 1},
	{IMX219_REG_PLL_VT_MPY, 32},		/* Pixel/Sys clock multiplier */
	{IMX219_REG_PLL_OP_MPY, 50},		/* Output clock multiplier */

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
	{IMX219_REG_CSI_DATA_FORMAT_A, (10 << 8) | 10}, /* 10-bits per pixels */

	/* Custom defaults */
	{IMX219_REG_BINNING_MODE_H, 0x00},
	{IMX219_REG_BINNING_MODE_V, 0x00},
	{IMX219_REG_DIGITAL_GAIN, 5000},
	{IMX219_REG_ANALOG_GAIN, 240},
	{IMX219_REG_INTEGRATION_TIME, 1600},
	{IMX219_REG_ORIENTATION, 0x03},

	{0},
};

#define IMX219_REGS_CROP(width, height)                                                            \
	{IMX219_REG_X_ADD_STA_A, (IMX219_FULL_WIDTH - (width)) / 2},                               \
	{IMX219_REG_X_ADD_END_A, (IMX219_FULL_WIDTH + (width)) / 2 - 1},                           \
	{IMX219_REG_Y_ADD_STA_A, (IMX219_FULL_HEIGHT - (height)) / 2},                             \
	{IMX219_REG_Y_ADD_END_A, (IMX219_FULL_HEIGHT + (height)) / 2 - 1}

static const struct video_reg size_1920x1080[] = {
	IMX219_REGS_CROP(1920, 1080),
	{IMX219_REG_X_OUTPUT_SIZE, 1920},
	{IMX219_REG_Y_OUTPUT_SIZE, 1080},
	{IMX219_REG_FRM_LENGTH_A, 1080 + 120},
	{0},
};

static const struct video_reg size_640x480[] = {
	IMX219_REGS_CROP(640, 480),
	{IMX219_REG_X_OUTPUT_SIZE, 640},
	{IMX219_REG_Y_OUTPUT_SIZE, 480},
	{IMX219_REG_FRM_LENGTH_A, 480 + 120},
	{0},
};

static const struct video_imager_mode modes_1920x1080[] = {
	{.fps = 30, .regs = {size_1920x1080}},
	{0},
};

static const struct video_imager_mode modes_640x480[] = {
	{.fps = 30, .regs = {size_640x480}},
	{0},
};

static const struct video_imager_mode *modes[] = {
	modes_1920x1080,
	modes_640x480,
	NULL,
};

static const struct video_format_cap fmts[] = {
	VIDEO_IMAGER_FORMAT_CAP(VIDEO_PIX_FMT_BGGR8, 1920, 1080),
	VIDEO_IMAGER_FORMAT_CAP(VIDEO_PIX_FMT_BGGR8, 640, 480),
	{0},
};

static int imx219_set_stream(const struct device *dev, bool on)
{
	const struct video_imager_config *cfg = dev->config;

	return video_write_cci_reg(&cfg->i2c, IMX219_REG_MODE_SELECT, on ? 0x01 : 0x00);
}

static int imx219_get_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_imager_config *cfg = dev->config;
	uint32_t reg;
	int ret;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		ret = video_read_cci_reg(&cfg->i2c, IMX219_REG_INTEGRATION_TIME, &reg);
		*(uint32_t *)value = reg;
		return ret;
	case VIDEO_CID_GAIN:
		ret = video_read_cci_reg(&cfg->i2c, IMX219_REG_ANALOG_GAIN, &reg);
		*(uint32_t *)value = reg;
		return ret;
	default:
		LOG_WRN("Control not supported");
		return -ENOTSUP;
	}
}

static int imx219_set_ctrl(const struct device *dev, unsigned int cid, void *value)
{
	const struct video_imager_config *cfg = dev->config;

	switch (cid) {
	case VIDEO_CID_EXPOSURE:
		/* Values for normal frame rate, different range for low frame rate mode */
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_INTEGRATION_TIME, (int)value);
	case VIDEO_CID_GAIN:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_ANALOG_GAIN, (int)value);
	case VIDEO_CID_TEST_PATTERN:
		return video_write_cci_reg(&cfg->i2c, IMX219_REG_TESTPATTERN, (int)value);
	default:
		LOG_WRN("Control not supported");
		return -ENOTSUP;
	}
}

static const DEVICE_API(video, imx219_driver_api) = {
	/* Local implementation */
	.set_stream = imx219_set_stream,
	.set_ctrl = imx219_set_ctrl,
	.get_ctrl = imx219_get_ctrl,
	/* Default implementation */
	.set_format = video_imager_set_fmt,
	.get_format = video_imager_get_fmt,
	.get_caps = video_imager_get_caps,
	.set_frmival = video_imager_set_frmival,
	.get_frmival = video_imager_get_frmival,
	.enum_frmival = video_imager_enum_frmival,
};

static int imx219_init(const struct device *dev)
{
	const struct video_imager_config *cfg = dev->config;
	uint32_t reg;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	k_sleep(K_MSEC(1));

	ret = video_write_cci_reg(&cfg->i2c, IMX219_REG_SOFTWARE_RESET, 1);
	if (ret != 0) {
		goto err;
	}

	/* Initializing time of silicon (t5): 32000 clock cycles, 5.3 msec for 6 MHz */
	k_sleep(K_MSEC(6));

	ret = video_read_cci_reg(&cfg->i2c, IMX219_REG_CHIP_ID, &reg);
	if (ret != 0) {
		LOG_ERR("Unable to read Chip ID");
		goto err;
	}

	if (reg != 0x0219) {
		LOG_ERR("Wrong chip ID %04x", reg);
		return -ENODEV;
	}

	return video_imager_init(dev, init_regs, 0);
err:
	LOG_ERR("Error during %s initialization: %s", dev->name, strerror(-ret));
	return ret;
}

#define IMX219_INIT(inst)                                                                          \
	static struct video_imager_data imx219_data_##inst;                                        \
                                                                                                   \
	static const struct video_imager_config imx219_cfg_##inst = {                              \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.fmts = fmts,                                                                      \
		.modes = modes,                                                                    \
		.data = &imx219_data_##inst,                                                       \
		.write_multi = video_write_cci_multi,                                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &imx219_init, NULL, NULL, &imx219_cfg_##inst, POST_KERNEL,     \
			      CONFIG_VIDEO_INIT_PRIORITY, &imx219_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IMX219_INIT)
