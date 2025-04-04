/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aptina_mt9m114

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/i2c.h>

#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_mt9m114, CONFIG_VIDEO_LOG_LEVEL);

#define MT9M114_CHIP_ID_VAL 0x2481

/* Sysctl registers */
#define MT9M114_CHIP_ID                    0x0000
#define MT9M114_COMMAND_REGISTER           0x0080
#define MT9M114_COMMAND_REGISTER_SET_STATE (1 << 1)
#define MT9M114_COMMAND_REGISTER_OK        (1 << 15)
#define MT9M114_RST_AND_MISC_CONTROL       0x001A

/* Camera Control registers */
#define MT9M114_CAM_SENSOR_CFG_Y_ADDR_START     0xC800
#define MT9M114_CAM_SENSOR_CFG_X_ADDR_START     0xC802
#define MT9M114_CAM_SENSOR_CFG_Y_ADDR_END       0xC804
#define MT9M114_CAM_SENSOR_CFG_X_ADDR_END       0xC806
#define MT9M114_CAM_SENSOR_CFG_CPIPE_LAST_ROW   0xC818
#define MT9M114_CAM_SENSOR_CTRL_READ_MODE       0xC834
#define MT9M114_CAM_CROP_WINDOW_WIDTH           0xC858
#define MT9M114_CAM_CROP_WINDOW_HEIGHT          0xC85A
#define MT9M114_CAM_OUTPUT_WIDTH                0xC868
#define MT9M114_CAM_OUTPUT_HEIGHT               0xC86A
#define MT9M114_CAM_OUTPUT_FORMAT               0xC86C
#define MT9M114_CAM_STAT_AWB_CLIP_WINDOW_XEND   0xC918
#define MT9M114_CAM_STAT_AWB_CLIP_WINDOW_YEND   0xC91A
#define MT9M114_CAM_STAT_AE_INITIAL_WINDOW_XEND 0xC920
#define MT9M114_CAM_STAT_AE_INITIAL_WINDOW_YEND 0xC922

/* System Manager registers */
#define MT9M114_SYSMGR_NEXT_STATE 0xDC00

/* System States */
#define MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE 0x28
#define MT9M114_SYS_STATE_START_STREAMING     0x34
#define MT9M114_SYS_STATE_ENTER_SUSPEND       0x40

/* Camera output format */
#define MT9M114_CAM_OUTPUT_FORMAT_FORMAT_YUV (0 << 8)
#define MT9M114_CAM_OUTPUT_FORMAT_FORMAT_RGB (1 << 8)

/* Camera control masks */
#define MT9M114_CAM_SENSOR_CTRL_HORZ_FLIP_EN BIT(0)
#define MT9M114_CAM_SENSOR_CTRL_VERT_FLIP_EN BIT(1)

struct mt9m114_config {
	struct i2c_dt_spec i2c;
};

struct mt9m114_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct mt9m114_data {
	struct mt9m114_ctrls ctrls;
	struct video_format fmt;
};

struct mt9m114_reg {
	uint16_t addr;
	uint16_t value_size;
	uint32_t value;
};

struct mt9m114_resolution_config {
	uint16_t width;
	uint16_t height;
	struct mt9m114_reg *params;
};

static struct mt9m114_reg mt9m114_init_config[] = {
	{0x098E, 2, 0x1000},    /* LOGICAL_ADDRESS_ACCESS */
	{0xC97E, 1, 0x01},      /* CAM_SYSCTL_PLL_ENABLE */
	{0xC980, 2, 0x0120},    /* CAM_SYSCTL_PLL_DIVIDER_M_N = 288 */
	{0xC982, 2, 0x0700},    /* CAM_SYSCTL_PLL_DIVIDER_P = 1792 */
	{0xC808, 4, 0x2DC6C00}, /* CAM_SENSOR_CFG_PIXCLK = 48 Mhz */
	{0x316A, 2, 0x8270},    /* Auto txlo_row for hot pixel and linear full well optimization */
	{0x316C, 2, 0x8270},    /* Auto txlo for hot pixel and linear full well optimization */
	{0x3ED0, 2, 0x2305},    /* Eclipse setting, ecl range=1, ecl value=2, ivln=3 */
	{0x3ED2, 2, 0x77CF},    /* TX_hi = 12 */
	{0x316E, 2, 0x8202}, /* Auto ecl , threshold 2x, ecl=0 at high gain, ecl=2 for low gain */
	{0x3180, 2, 0x87FF}, /* Enable delta dark */
	{0x30D4, 2, 0x6080}, /* Disable column correction due to AE oscillation problem */
	{0xA802, 2, 0x0008}, /* RESERVED_AE_TRACK_02 */
	{0x3E14, 2, 0xFF39}, /* Enabling pixout clamping to VAA to solve column band issue */
	{0xC80C, 2, 0x0001}, /* CAM_SENSOR_CFG_ROW_SPEED */
	{0xC80E, 2, 0x00DB}, /* CAM_SENSOR_CFG_FINE_INTEG_TIME_MIN = 219 */
	{0xC810, 2, 0x07C2}, /* CAM_SENSOR_CFG_FINE_INTEG_TIME_MAX = 1986 */
	{0xC812, 2, 0x02FE}, /* CAM_SENSOR_CFG_FRAME_LENGTH_LINES = 766 */
	{0xC814, 2, 0x0845}, /* CAM_SENSOR_CFG_LINE_LENGTH_PCK = 2117 */
	{0xC816, 2, 0x0060}, /* CAM_SENSOR_CFG_FINE_CORRECTION = 96 */
	{0xC826, 2, 0x0020}, /* CAM_SENSOR_CFG_REG_0_DATA = 32 */
	{0xC834, 2, 0x0000}, /* CAM_SENSOR_CONTROL_READ_MODE */
	{0xC854, 2, 0x0000}, /* CAM_CROP_WINDOW_XOFFSET */
	{0xC856, 2, 0x0000}, /* CAM_CROP_WINDOW_YOFFSET */
	{0xC85C, 1, 0x03},   /* CAM_CROP_CROPMODE */
	{0xC878, 1, 0x00},   /* CAM_AET_AEMODE */
	{0xC88C, 2, 0x1D9A}, /* CAM_AET_MAX_FRAME_RATE = 7578 */
	{0xC88E, 2, 0x1D9A}, /* CAM_AET_MIN_FRAME_RATE = 7578 */
	{0xC914, 2, 0x0000}, /* CAM_STAT_AWB_CLIP_WINDOW_XSTART */
	{0xC916, 2, 0x0000}, /* CAM_STAT_AWB_CLIP_WINDOW_YSTART */
	{0xC91C, 2, 0x0000}, /* CAM_STAT_AE_INITIAL_WINDOW_XSTART */
	{0xC91E, 2, 0x0000}, /* CAM_STAT_AE_INITIAL_WINDOW_YSTART */
	{0x001E, 2, 0x0777}, /* REG_PAD_SLEW */
	{0xC86E, 2, 0x0038}, /* CAM_OUTPUT_FORMAT_YUV_CLIP for CSI */
	{0xC984, 2, 0x8000}, /* CAM_PORT_OUTPUT_CONTROL, for MIPI CSI-2 interface : 0x8000 */
	{/* NULL terminated */}};

static struct mt9m114_reg mt9m114_480_272[] = {
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_START, 2, 0x00D4},     /* 212 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_START, 2, 0x00A4},     /* 164 */
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_END, 2, 0x02FB},       /* 763 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_END, 2, 0x046B},       /* 1131 */
	{MT9M114_CAM_SENSOR_CFG_CPIPE_LAST_ROW, 2, 0x0223},   /* 547 */
	{MT9M114_CAM_CROP_WINDOW_WIDTH, 2, 0x03C0},           /* 960 */
	{MT9M114_CAM_CROP_WINDOW_HEIGHT, 2, 0x0220},          /* 544 */
	{MT9M114_CAM_OUTPUT_WIDTH, 2, 0x01E0},                /* 480 */
	{MT9M114_CAM_OUTPUT_HEIGHT, 2, 0x0110},               /* 272 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_XEND, 2, 0x01DF},   /* 479 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_YEND, 2, 0x010F},   /* 271 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_XEND, 2, 0x005F}, /* 95 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_YEND, 2, 0x0035}, /* 53 */
	{/* NULL terminated */}};

static struct mt9m114_reg mt9m114_640_480[] = {
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_START, 2, 0x0000},     /* 0 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_START, 2, 0x0000},     /* 0 */
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_END, 2, 0x03CD},       /* 973 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_END, 2, 0x050D},       /* 1293 */
	{MT9M114_CAM_SENSOR_CFG_CPIPE_LAST_ROW, 2, 0x01E3},   /* 483 */
	{MT9M114_CAM_CROP_WINDOW_WIDTH, 2, 0x0280},           /* 640 */
	{MT9M114_CAM_CROP_WINDOW_HEIGHT, 2, 0x01E0},          /* 480 */
	{MT9M114_CAM_OUTPUT_WIDTH, 2, 0x0280},                /* 640 */
	{MT9M114_CAM_OUTPUT_HEIGHT, 2, 0x01E0},               /* 480 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_XEND, 2, 0x027F},   /* 639 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_YEND, 2, 0x01DF},   /* 479 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_XEND, 2, 0x007F}, /* 127 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_YEND, 2, 0x005F}, /* 95 */
	{/* NULL terminated */}};

static struct mt9m114_reg mt9m114_1280_720[] = {
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_START, 2, 0x007C},     /* 124 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_START, 2, 0x0004},     /* 4 */
	{MT9M114_CAM_SENSOR_CFG_Y_ADDR_END, 2, 0x0353},       /* 851 */
	{MT9M114_CAM_SENSOR_CFG_X_ADDR_END, 2, 0x050B},       /* 1291 */
	{MT9M114_CAM_SENSOR_CFG_CPIPE_LAST_ROW, 2, 0x02D3},   /* 723 */
	{MT9M114_CAM_CROP_WINDOW_WIDTH, 2, 0x0500},           /* 1280 */
	{MT9M114_CAM_CROP_WINDOW_HEIGHT, 2, 0x02D0},          /* 720 */
	{MT9M114_CAM_OUTPUT_WIDTH, 2, 0x0500},                /* 1280 */
	{MT9M114_CAM_OUTPUT_HEIGHT, 2, 0x02D0},               /* 720 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_XEND, 2, 0x04FF},   /* 1279 */
	{MT9M114_CAM_STAT_AWB_CLIP_WINDOW_YEND, 2, 0x02CF},   /* 719 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_XEND, 2, 0x00FF}, /* 255 */
	{MT9M114_CAM_STAT_AE_INITIAL_WINDOW_YEND, 2, 0x008F}, /* 143 */
	{/* NULL terminated */}};

static struct mt9m114_resolution_config resolutionConfigs[] = {
	{.width = 480, .height = 272, .params = mt9m114_480_272},
	{.width = 640, .height = 480, .params = mt9m114_640_480},
	{.width = 1280, .height = 720, .params = mt9m114_1280_720},
};

#define MT9M114_VIDEO_FORMAT_CAP(width, height, format)                                            \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	MT9M114_VIDEO_FORMAT_CAP(480, 272, VIDEO_PIX_FMT_RGB565),
	MT9M114_VIDEO_FORMAT_CAP(480, 272, VIDEO_PIX_FMT_YUYV),
	MT9M114_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565),
	MT9M114_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),
	MT9M114_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_RGB565),
	MT9M114_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_YUYV),
	{0}};

static inline int i2c_burst_read16_dt(const struct i2c_dt_spec *spec, uint16_t start_addr,
				      uint8_t *buf, uint32_t num_bytes)
{
	uint8_t addr_buffer[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	return i2c_write_read_dt(spec, addr_buffer, sizeof(addr_buffer), buf, num_bytes);
}

static inline int i2c_burst_write16_dt(const struct i2c_dt_spec *spec, uint16_t start_addr,
				       const uint8_t *buf, uint32_t num_bytes)
{
	uint8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(spec, msg, 2);
}

static int mt9m114_write_reg(const struct device *dev, uint16_t reg_addr, uint8_t reg_size,
			     void *value)
{
	const struct mt9m114_config *cfg = dev->config;

	switch (reg_size) {
	case 2:
		*(uint16_t *)value = sys_cpu_to_be16(*(uint16_t *)value);
		break;
	case 4:
		*(uint32_t *)value = sys_cpu_to_be32(*(uint32_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return i2c_burst_write16_dt(&cfg->i2c, reg_addr, value, reg_size);
}

static int mt9m114_read_reg(const struct device *dev, uint16_t reg_addr, uint8_t reg_size,
			    void *value)
{
	const struct mt9m114_config *cfg = dev->config;
	int err;

	if (reg_size > 4) {
		return -ENOTSUP;
	}

	err = i2c_burst_read16_dt(&cfg->i2c, reg_addr, value, reg_size);
	if (err) {
		return err;
	}

	switch (reg_size) {
	case 2:
		*(uint16_t *)value = sys_be16_to_cpu(*(uint16_t *)value);
		break;
	case 4:
		*(uint32_t *)value = sys_be32_to_cpu(*(uint32_t *)value);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mt9m114_modify_reg(const struct device *dev, const uint16_t addr,
			      uint8_t reg_size, const uint32_t mask, const uint32_t val)
{
	uint32_t oldVal = 0;
	uint32_t newVal = 0;

	int ret = mt9m114_read_reg(dev, addr, reg_size, &oldVal);

	if (ret) {
		return ret;
	}

	newVal = (oldVal & ~mask) | (val & mask);

	return mt9m114_write_reg(dev, addr, reg_size, &newVal);
}

static int mt9m114_write_all(const struct device *dev, struct mt9m114_reg *reg)
{
	int i = 0;

	while (reg[i].value_size) {
		int err;

		err = mt9m114_write_reg(dev, reg[i].addr, reg[i].value_size, &reg[i].value);
		if (err) {
			return err;
		}

		i++;
	}

	return 0;
}

static int mt9m114_software_reset(const struct device *dev)
{
	int ret = mt9m114_modify_reg(dev, MT9M114_RST_AND_MISC_CONTROL, 2, 0x01, 0x01);

	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(1));

	ret = mt9m114_modify_reg(dev, MT9M114_RST_AND_MISC_CONTROL, 2, 0x01, 0x00);
	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(45));

	return 0;
}

static int mt9m114_set_state(const struct device *dev, uint8_t state)
{
	uint16_t val;
	int err;

	/* Set next state. */
	mt9m114_write_reg(dev, MT9M114_SYSMGR_NEXT_STATE, 1, &state);

	/* Check that the FW is ready to accept a new command. */
	while (1) {
		err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
		if (err) {
			return err;
		}

		if (!(val & MT9M114_COMMAND_REGISTER_SET_STATE)) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	/* Issue the Set State command. */
	val = MT9M114_COMMAND_REGISTER_SET_STATE | MT9M114_COMMAND_REGISTER_OK;
	mt9m114_write_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);

	/* Wait for the FW to complete the command. */
	while (1) {
		err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
		if (err) {
			return err;
		}

		if (!(val & MT9M114_COMMAND_REGISTER_SET_STATE)) {
			break;
		}

		k_sleep(K_MSEC(1));
	}

	/* Check the 'OK' bit to see if the command was successful. */
	err = mt9m114_read_reg(dev, MT9M114_COMMAND_REGISTER, 2, &val);
	if (err || !(val & MT9M114_COMMAND_REGISTER_OK)) {
		return -EIO;
	}

	return 0;
}

static int mt9m114_set_output_format(const struct device *dev, int pixel_format)
{
	int ret = 0;
	uint16_t output_format;

	if (pixel_format == VIDEO_PIX_FMT_YUYV) {
		output_format = (MT9M114_CAM_OUTPUT_FORMAT_FORMAT_YUV | (1U << 1U));
	} else if (pixel_format == VIDEO_PIX_FMT_RGB565) {
		output_format = (MT9M114_CAM_OUTPUT_FORMAT_FORMAT_RGB | (1U << 1U));
	}

	ret = mt9m114_write_reg(dev, MT9M114_CAM_OUTPUT_FORMAT, sizeof(output_format),
				&output_format);

	return ret;
}

static int mt9m114_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->data;
	int ret;
	int i = 0;

	while (fmts[i].pixelformat) {
		if (fmt->pixelformat == fmts[i].pixelformat && fmt->width >= fmts[i].width_min &&
		    fmt->width <= fmts[i].width_max && fmt->height >= fmts[i].height_min &&
		    fmt->height <= fmts[i].height_max) {
			break;
		}
		i++;
	}

	if (i == (ARRAY_SIZE(fmts) - 1)) {
		LOG_ERR("Unsupported pixel format or resolution");
		return -ENOTSUP;
	}

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		/* nothing to do */
		return 0;
	}

	drv_data->fmt = *fmt;

	/* Set output pixel format */
	ret = mt9m114_set_output_format(dev, fmt->pixelformat);
	if (ret) {
		LOG_ERR("Unable to set pixel format");
		return ret;
	}

	/* Set output resolution */
	for (i = 0; i < ARRAY_SIZE(resolutionConfigs); i++) {
		if (fmt->width == resolutionConfigs[i].width &&
		    fmt->height == resolutionConfigs[i].height) {
			ret = mt9m114_write_all(dev, resolutionConfigs[i].params);
			if (ret) {
				LOG_ERR("Unable to set resolution");
				return ret;
			}

			break;
		}
	}

	/* Apply Config */
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE);
}

static int mt9m114_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			   struct video_format *fmt)
{
	struct mt9m114_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int mt9m114_set_stream(const struct device *dev, bool enable)
{
	return enable ? mt9m114_set_state(dev, MT9M114_SYS_STATE_START_STREAMING)
		      : mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_SUSPEND);
}

static int mt9m114_get_caps(const struct device *dev, enum video_endpoint_id ep,
			    struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int mt9m114_set_ctrl(const struct device *dev, uint32_t id)
{
	int ret = 0;
	struct mt9m114_data *drv_data = dev->data;

	switch (id) {
	case VIDEO_CID_HFLIP:
		ret = mt9m114_modify_reg(
			dev, MT9M114_CAM_SENSOR_CTRL_READ_MODE, 2,
			MT9M114_CAM_SENSOR_CTRL_HORZ_FLIP_EN,
			drv_data->ctrls.hflip.val ? MT9M114_CAM_SENSOR_CTRL_HORZ_FLIP_EN : 0);
		break;
	case VIDEO_CID_VFLIP:
		ret = mt9m114_modify_reg(
			dev, MT9M114_CAM_SENSOR_CTRL_READ_MODE, 2,
			MT9M114_CAM_SENSOR_CTRL_VERT_FLIP_EN,
			drv_data->ctrls.vflip.val ? MT9M114_CAM_SENSOR_CTRL_VERT_FLIP_EN : 0);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		return ret;
	}

	/* Apply Config */
	return mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE);
}

static DEVICE_API(video, mt9m114_driver_api) = {
	.set_format = mt9m114_set_fmt,
	.get_format = mt9m114_get_fmt,
	.get_caps = mt9m114_get_caps,
	.set_stream = mt9m114_set_stream,
	.set_ctrl = mt9m114_set_ctrl,
};

static int mt9m114_init_controls(const struct device *dev)
{
	int ret;
	struct mt9m114_data *drv_data = dev->data;
	struct mt9m114_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int mt9m114_init(const struct device *dev)
{
	struct video_format fmt;
	uint16_t val;
	int ret;

	/* no power control, wait for camera ready */
	k_sleep(K_MSEC(100));

	ret = mt9m114_read_reg(dev, MT9M114_CHIP_ID, sizeof(val), &val);
	if (ret) {
		LOG_ERR("Unable to read chip ID");
		return -ENODEV;
	}

	if (val != MT9M114_CHIP_ID_VAL) {
		LOG_ERR("Wrong ID: %04x (exp %04x)", val, MT9M114_CHIP_ID_VAL);
		return -ENODEV;
	}

	/* SW reset */
	mt9m114_software_reset(dev);

	/* Init registers */
	ret = mt9m114_write_all(dev, mt9m114_init_config);
	if (ret) {
		LOG_ERR("Unable to initialize mt9m114 config");
		return ret;
	}

	/* Set default format to 480x272 RGB565 */
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 480;
	fmt.height = 272;
	fmt.pitch = fmt.width * 2;

	ret = mt9m114_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}

	/* Suspend any stream */
	mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_SUSPEND);

	/* Initialize controls */
	return mt9m114_init_controls(dev);
}

#if 1 /* Unique Instance */

static const struct mt9m114_config mt9m114_cfg_0 = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
};

static struct mt9m114_data mt9m114_data_0;

static int mt9m114_init_0(const struct device *dev)
{
	const struct mt9m114_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	return mt9m114_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &mt9m114_init_0, NULL, &mt9m114_data_0, &mt9m114_cfg_0, POST_KERNEL,
		      CONFIG_VIDEO_INIT_PRIORITY, &mt9m114_driver_api);

VIDEO_DEVICE_DEFINE(mt9m114, DEVICE_DT_INST_GET(0), NULL);

#endif
