/* mt9m114.c - driver for mt9m114 image sensor */

/*
 * Copyright (c) 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <init.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/image_sensor.h>
#include <misc/__assert.h>

#define LOG_LEVEL LOG_LEVEL_ERR
#include <logging/log.h>
LOG_MODULE_REGISTER(MT9M114);

#define MT9M114_DBG

#undef LOG_ERR
#define LOG_ERR printk

#ifdef MT9M114_DBG
#undef LOG_INF
#define LOG_INF printk
#endif

#include "mt9m114.h"

static const struct img_sensor_reg mt9m114_480_272[] = {
	{MT9M114_VAR_CAM_SENSOR_CFG_Y_ADDR_START, W2B,
		0x00D4, W2B},
	/* cam_sensor_cfg_y_addr_start = 212 */
	{MT9M114_VAR_CAM_SENSOR_CFG_X_ADDR_START, W2B,
		0x00A4, W2B},
	/* cam_sensor_cfg_x_addr_start = 164 */
	{MT9M114_VAR_CAM_SENSOR_CFG_Y_ADDR_END, W2B,
		0x02FB, W2B},
	/* cam_sensor_cfg_y_addr_end = 763 */
	{MT9M114_VAR_CAM_SENSOR_CFG_X_ADDR_END, W2B,
		0x046B, W2B},
	/* cam_sensor_cfg_x_addr_end = 1131 */
	{MT9M114_VAR_CAM_SENSOR_CFG_CPIPE_LAST_ROW, W2B,
		0x0223, W2B},
	/* cam_sensor_cfg_cpipe_last_row = 547 */
	{MT9M114_VAR_CAM_CROP_WINDOW_WIDTH, W2B,
		0x03C0, W2B},
	/* cam_crop_window_width = 960 */
	{MT9M114_VAR_CAM_CROP_WINDOW_HEIGHT, W2B,
		0x0220, W2B},
	/* cam_crop_window_height = 544 */
	{MT9M114_VAR_CAM_OUTPUT_WIDTH, W2B,
		0x01E0, W2B},
	/* cam_output_width = 480 */
	{MT9M114_VAR_CAM_OUTPUT_HEIGHT, W2B,
		0x0110, W2B},
	/* cam_output_height = 272 */
	{MT9M114_VAR_CAM_STAT_AWB_CLIP_WINDOW_XEND, W2B,
		0x01DF, W2B},
	/* cam_stat_awb_clip_window_xend = 479 */
	{MT9M114_VAR_CAM_STAT_AWB_CLIP_WINDOW_YEND, W2B,
		0x010F, W2B},
	/* cam_stat_awb_clip_window_yend = 271 */
	{MT9M114_VAR_CAM_STAT_AE_INITIAL_WINDOW_XEND, W2B,
		0x005F, W2B},
	/* cam_stat_ae_initial_window_xend = 95 */
	{MT9M114_VAR_CAM_STAT_AE_INITIAL_WINDOW_YEND, W2B,
		0x0035, W2B},
	/* cam_stat_ae_initial_window_yend = 53 */
};

static const struct img_sensor_reg mt9m114_init_cfg[] = {
	{MT9M114_REG_LOGICAL_ADDRESS_ACCESS, W2B,
		0x1000, W2B},
	/* PLL Fout = (Fin * 2 * m) / ((n + 1) * (p + 1)) */
	{MT9M114_VAR_CAM_SYSCTL_PLL_ENABLE, W2B,
		0x01, W1B},
	/*  cam_sysctl_pll_enable = 1 */
	{MT9M114_VAR_CAM_SYSCTL_PLL_DIVIDER_M_N, W2B,
		0x0120, W2B},
	/*  cam_sysctl_pll_divider_m_n = 288 */
	{MT9M114_VAR_CAM_SYSCTL_PLL_DIVIDER_P, W2B,
		0x0700, W2B},
	/*  cam_sysctl_pll_divider_p = 1792 */
	{MT9M114_VAR_CAM_SENSOR_CFG_PIXCLK, W2B,
		0x2DC6C00, W4B},
	/*  cam_sensor_cfg_pixclk = 48000000 */
	{0x316A, W2B,
		0x8270, W2B},
	/*  auto txlo_row for hot pixel and linear full well optimization */
	{0x316C, W2B,
		0x8270, W2B},
	/*  auto txlo for hot pixel and linear full well optimization */
	{0x3ED0, W2B,
		0x2305, W2B},
	/*  eclipse setting, ecl range=1, ecl value=2, ivln=3 */
	{0x3ED2, W2B,
		0x77CF, W2B},
	/*  TX_hi=12 */
	{0x316E, W2B,
		0x8202, W2B},
	/*  auto ecl , threshold 2x, ecl=0 at high gain, ecl=2 for low gain */
	{0x3180, W2B,
		0x87FF, W2B},
	/*  enable delta dark */
	{0x30D4, W2B,
		0x6080, W2B},
	/*  disable column correction due to AE oscillation problem */
	{0xA802, W2B,
		0x0008, W2B},
	/*  RESERVED_AE_TRACK_02 */
	{0x3E14, W2B,
		0xFF39, W2B},
	/* Enabling pixout clamping to VAA during ADC streaming to
	 * solve column band issue
	 */
	{MT9M114_VAR_CAM_SENSOR_CFG_ROW_SPEED, W2B,
		0x0001, W2B},
	/*  cam_sensor_cfg_row_speed = 1 */
	{MT9M114_VAR_CAM_SENSOR_CFG_FINE_INTEG_TIME_MIN, W2B,
		0x00DB, W2B},
	/*  cam_sensor_cfg_fine_integ_time_min = 219 */
	{MT9M114_VAR_CAM_SENSOR_CFG_FINE_INTEG_TIME_MAX, W2B,
		0x07C2, W2B},
	/*  cam_sensor_cfg_fine_integ_time_max = 1986 */
	{MT9M114_VAR_CAM_SENSOR_CFG_FRAME_LENGTH_LINES, W2B,
		0x02FE, W2B},
	/*  cam_sensor_cfg_frame_length_lines = 766 */
	{MT9M114_VAR_CAM_SENSOR_CFG_LINE_LENGTH_PCK, W2B,
		0x0845, W2B},
	/*  cam_sensor_cfg_line_length_pck = 2117 */
	{MT9M114_VAR_CAM_SENSOR_CFG_FINE_CORRECTION, W2B,
		0x0060, W2B},
	/*  cam_sensor_cfg_fine_correction = 96 */
	{MT9M114_VAR_CAM_SENSOR_CFG_REG_0_DATA, W2B,
		0x0020, W2B},
	/*  cam_sensor_cfg_reg_0_data = 32 */
	{MT9M114_VAR_CAM_SENSOR_CONTROL_READ_MODE, W2B,
		0x0000, W2B},
	/*  cam_sensor_control_read_mode = 0 */
	{MT9M114_VAR_CAM_CROP_WINDOW_XOFFSET, W2B,
		0x0000, W2B},
	/*  cam_crop_window_xoffset = 0 */
	{MT9M114_VAR_CAM_CROP_WINDOW_YOFFSET, W2B,
		0x0000, W2B},
	/*  cam_crop_window_yoffset = 0 */
	{MT9M114_VAR_CAM_CROP_CROPMODE, W2B,
		0x03, W1B},
	/*  cam_crop_cropmode = 3 */
	{MT9M114_VAR_CAM_AET_AEMODE, W2B,
		0x00, W1B},
	/*  cam_aet_aemode = 0 */
	{MT9M114_VAR_CAM_AET_MAX_FRAME_RATE, W2B,
		0x1D9A, W2B},
	/*  cam_aet_max_frame_rate = 7578 */
	{MT9M114_VAR_CAM_AET_MIN_FRAME_RATE, W2B,
		0x1D9A, W2B},
	/*  cam_aet_min_frame_rate = 7578 */
	{MT9M114_VAR_CAM_STAT_AWB_CLIP_WINDOW_XSTART, W2B,
		0x0000, W2B},
	/*  cam_stat_awb_clip_window_xstart = 0 */
	{MT9M114_VAR_CAM_STAT_AWB_CLIP_WINDOW_YSTART, W2B,
		0x0000, W2B},
	/*  cam_stat_awb_clip_window_ystart = 0 */
	{MT9M114_VAR_CAM_STAT_AE_INITIAL_WINDOW_XSTART, W2B,
		0x0000, W2B},
	/*  cam_stat_ae_initial_window_xstart = 0 */
	{MT9M114_VAR_CAM_STAT_AE_INITIAL_WINDOW_YSTART, W2B,
		0x0000, W2B},
	/*  cam_stat_ae_initial_window_ystart = 0 */
	{MT9M114_REG_PAD_SLEW, W2B,
		0x0777, W2B},
	/*  Pad slew rate */
	{MT9M114_VAR_CAM_OUTPUT_FORMAT_YUV, W2B,
		0x0038, W2B},
	/*  Must set cam_output_format_yuv_clip for CSI */
};

static int mt9m114_read_reg(struct device *dev,
	u16_t reg_addr, u8_t *reg_data, u8_t len)
{
	int ret, i = 0;
	u8_t addr_buffer[MT9M114_REG_ADDR_LEN];
	u8_t data_buffer[4];
	struct img_sensor_data *drv_data = dev->driver_data;

	assert(len <= 4);
	addr_buffer[1] = reg_addr & 0xFF;
	addr_buffer[0] = reg_addr >> 8;
	ret = i2c_write_read(drv_data->host_info.i2c,
		drv_data->client_info.i2c_addr,
		addr_buffer, MT9M114_REG_ADDR_LEN,
		data_buffer, len);
	if (!ret) {
		while (len) {
			len--;
			reg_data[i] = data_buffer[len];
			i++;
		}
	}

	return ret;
}

static int mt9m114_write_reg(struct device *dev,
	u16_t reg_addr, u32_t reg_data, u8_t len)
{
	int ret;
	u8_t data[MT9M114_REG_ADDR_LEN + 4];
	u8_t data_len = len;
	struct img_sensor_data *drv_data = dev->driver_data;

	assert(len <= 4);
	data[1] = reg_addr & 0xFF;
	data[0] = reg_addr >> 8;
	while (data_len) {
		data_len--;
		data[data_len + MT9M114_REG_ADDR_LEN] = (u8_t)reg_data;
		reg_data >>= 8;
	}
	ret = i2c_write(drv_data->host_info.i2c, (const u8_t *)data,
		MT9M114_REG_ADDR_LEN + len,
		drv_data->client_info.i2c_addr);

	return ret;
}

static int mt9m114_read_reg_cb(struct device *dev,
	u32_t reg_addr, u8_t reg_len __unused, u8_t *reg_data, u8_t data_len)
{
	return mt9m114_read_reg(dev, (u16_t)reg_addr, reg_data, data_len);
}

static int mt9m114_write_reg_cb(struct device *dev,
	u32_t reg_addr, u8_t reg_len __unused, u32_t reg_data, u8_t data_len)
{
	return mt9m114_write_reg(dev, (u16_t)reg_addr, reg_data, data_len);
}

static int mt9m114_modify_reg(struct device *dev,
	u32_t reg, u8_t data_width,	u32_t clr_msk, u32_t value)
{
	int status;
	u32_t regval;

	if (data_width != 1 && data_width != 2 && data_width != 4) {
		return -EINVAL;
	}

	status = mt9m114_read_reg(dev, reg, (u8_t *)&regval, data_width);
	if (status) {
		return status;
	}

	regval = (regval & ~(clr_msk)) | (value & clr_msk);

	status = mt9m114_write_reg(dev, reg, regval, data_width);
	return status;
}

static int mt9m114_soft_reset(struct device *dev)
{
	int ret = mt9m114_modify_reg(dev,
		MT9M114_REG_RESET_AND_MISC_CONTROL, 2u, 0x01, 0x01);

	if (ret) {
		return ret;
	}

	k_busy_wait(1000);
	ret = mt9m114_modify_reg(dev,
		MT9M114_REG_RESET_AND_MISC_CONTROL, 2u, 0x01, 0x00);
	k_busy_wait(45 * 1000);

	return ret;
}

static int mt9m114_multi_write(struct device *dev,
	const struct img_sensor_reg regs[], int num)
{
	int status = 0, i;

	for (i = 0; i < num; i++) {
		status = mt9m114_write_reg(dev, regs[i].reg,
			regs[i].value, (u8_t)regs[i].w_value);
		if (status) {
			break;
		}
	}
	return status;
}

static int mt9m114_set_state(struct device *dev,
	u8_t next_state)
{
	u16_t value;
	int ret;

	/* Set the desired next state. */
	ret = mt9m114_write_reg(dev, MT9M114_VAR_SYSMGR_NEXT_STATE,
							next_state, 1);
	if (ret) {
		return ret;
	}

	/* Check that the FW is ready to accept a new command. */
	while (1) {
		k_busy_wait(100);
		ret = mt9m114_read_reg(dev,
			MT9M114_REG_COMMAND_REGISTER,
			(u8_t *)&value, 2);
		if (ret)
			return ret;
		if (!(value & MT9M114_COMMAND_SET_STATE)) {
			break;
		}
	}

	/* Issue the Set State command. */
	ret = mt9m114_write_reg(dev, MT9M114_REG_COMMAND_REGISTER,
		MT9M114_COMMAND_SET_STATE | MT9M114_COMMAND_OK, 2);
	if (ret) {
		return ret;
	}

	/* Wait for the FW to complete the command. */
	while (1) {
		k_busy_wait(100);
		ret = mt9m114_read_reg(dev,
			MT9M114_REG_COMMAND_REGISTER,
			(u8_t *)&value, 2);
		if (ret) {
			return ret;
		}
		if (!(value & MT9M114_COMMAND_SET_STATE)) {
			break;
		}
	}

	/* Check the 'OK' bit to see if the command was successful. */
	ret = mt9m114_read_reg(dev,
		MT9M114_REG_COMMAND_REGISTER,
		(u8_t *)&value, 2);
	if (ret) {
		return ret;
	}
	if (!(value & MT9M114_COMMAND_OK)) {
		return -EIO;
	}

	return ret;
}

static int mt9m114_reset(struct device *dev)
{
	int ret;

	/* SW reset. */
	ret = mt9m114_soft_reset(dev);
	if (ret) {
		return ret;
	}

	ret = mt9m114_multi_write(dev, mt9m114_init_cfg,
		ARRAY_SIZE(mt9m114_init_cfg));
	if (ret) {
		return ret;
	}

	return ret;
}

static int mt9m114_set_config(struct device *dev)
{
	struct img_sensor_data *drv_data = dev->driver_data;
	u16_t pixformat = 0;
	int ret;

	/* Pixel format. */
	if (PIXEL_FORMAT_RGB_565 ==
		drv_data->client_info.pixformat) {
		pixformat |= ((1U << 8U) | (1U << 1U));
	}

	ret = mt9m114_write_reg(dev,
		MT9M114_VAR_CAM_OUTPUT_FORMAT, pixformat, 2);
	if (ret) {
		return ret;
	}

	ret = mt9m114_write_reg(dev,
		MT9M114_VAR_CAM_PORT_OUTPUT_CONTROL, 0x8000, 2);
	if (ret) {
		return ret;
	}

	/* Resolution 480 * 272*/
	if (drv_data->client_info.width == MT9M114_DEFAULT_WIDTH &&
		drv_data->client_info.height == MT9M114_DEFAULT_HEIGHT) {
		ret = mt9m114_multi_write(dev,
			mt9m114_480_272, ARRAY_SIZE(mt9m114_480_272));
		if (ret) {
			return ret;
		}
	} else {
		LOG_ERR("MT9M114: other than 480X272 not implemented\r\n");
		return -EINVAL;
	}

	/* Execute Change-Config command. */
	ret = mt9m114_set_state(dev, MT9M114_SYS_STATE_ENTER_CONFIG_CHANGE);
	if (ret) {
		return ret;
	}

	ret = mt9m114_set_state(dev, MT9M114_SYS_STATE_START_STREAMING);

	return ret;
}

static int mt9m114_set_pixformat(struct device *dev,
	enum display_pixel_format pixformat)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	if (pixformat != PIXEL_FORMAT_RGB_565) {
		LOG_ERR("Other than RGB565 not implemented on mt9m114\r\n");
		return -EINVAL;
	}
	drv_data->client_info.pixformat = pixformat;

	return 0;
}

static int mt9m114_set_framesize(struct device *dev,
	u16_t width, u16_t height)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	if (width != MT9M114_DEFAULT_WIDTH ||
		height != MT9M114_DEFAULT_HEIGHT) {
		LOG_ERR("Other than 480X272 not implemented"
			" on mt9m114 %d X %d\r\n",
			width, height);
		return -EINVAL;
	}
	drv_data->client_info.width = width;
	drv_data->client_info.height = height;

	return 0;
}

static int mt9m114_set_contrast(struct device *dev, int level)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	drv_data->client_info.contrast_level = level;

	return 0;
}

static int mt9m114_set_brightness(struct device *dev, int level)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	drv_data->client_info.bright_level = level;

	return 0;
}

static int mt9m114_set_effect(struct device *dev, enum img_sensor_effect effect)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	drv_data->client_info.effect = effect;

	return 0;
}

static int mt9m114_set_gain(struct device *dev, float r_gain_db,
	float g_gain_db, float b_gain_db)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	drv_data->client_info.r_gain_db = r_gain_db;
	drv_data->client_info.g_gain_db = g_gain_db;
	drv_data->client_info.b_gain_db = b_gain_db;

	return 0;
}

static int mt9m114_get_cap(struct device *dev,
	struct img_sensor_capability *cap)
{
	struct img_sensor_data *drv_data = dev->driver_data;

	cap->pixformat_support =
		drv_data->client_info.cap.pixformat_support;
	cap->width_max = drv_data->client_info.cap.width_max;
	cap->height_max = drv_data->client_info.cap.height_max;

	return 0;
}


const struct img_sensor_driver_api mt9m114_api = {
	.img_sensor_reset_cb = mt9m114_reset,
	.img_sensor_get_cap_cb = mt9m114_get_cap,
	.img_sensor_read_reg_cb = mt9m114_read_reg_cb,
	.img_sensor_write_reg_cb = mt9m114_write_reg_cb,
	.img_sensor_set_pixformat_cb = mt9m114_set_pixformat,
	.img_sensor_set_framesize_cb = mt9m114_set_framesize,
	.img_sensor_set_contrast_cb = mt9m114_set_contrast,
	.img_sensor_set_brightness_cb = mt9m114_set_brightness,
	.img_sensor_set_rgb_gain_cb = mt9m114_set_gain,
	.img_sensor_set_effect_cb = mt9m114_set_effect,
	.img_sensor_config_cb = mt9m114_set_config,
};

struct img_sensor_info mt9m114_info = {
	.sensor_client = {
		.i2c_addr = MT9M114_I2C_ADDR,
		.sensor_id = MT9M114_CHIP_ID,
		.w_sensor_id = W1B,
		.id_reg = MT9M114_REG_CHIP_ID,
		.w_id_reg = W2B,
		.width = MT9M114_DEFAULT_WIDTH,
		.height = MT9M114_DEFAULT_HEIGHT,
		.pixformat = PIXEL_FORMAT_RGB_565,
		.cap = {
			.width_max = MT9M114_MAX_WIDTH,
			.height_max = MT9M114_MAX_HEIGHT,
			.pixformat_support = (
				PIXEL_FORMAT_RGB_565 |
				PIXEL_FORMAT_YUV_420 |
				PIXEL_FORMAT_YUV_422),
		},
	},
	.sensor_api = &mt9m114_api,
};

static int mt9m114_dev_init(struct device *dev)
{
	struct img_sensor_info *drv_data = dev->driver_data;

	img_sensor_support_add(drv_data);

	return 0;
}

DEVICE_AND_API_INIT(mt9m114_dev,
		CONFIG_MT9M114_NAME, mt9m114_dev_init,
		&mt9m114_info, NULL, POST_KERNEL,
		CONFIG_IMAGE_SENSOR_INIT_PRIO,
		0);

