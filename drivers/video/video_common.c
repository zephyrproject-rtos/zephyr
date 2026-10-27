/*
 * Copyright (c) 2019, Linaro Limited
 * SPDX-FileCopyrightText: Copyright tinyVision.ai Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/video/video.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_common, CONFIG_VIDEO_LOG_LEVEL);

struct video_device *video_find_vdev(const struct device *dev)
{
	if (!dev) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(video_device, vdev) {
		if (vdev->dev == dev) {
			return vdev;
		}
	}

	return NULL;
}

static const char *const *video_get_std_menu_ctrl(uint32_t id)
{
	static char const *const power_line_frequency[] = {
		"Disabled", "50 Hz", "60 Hz", "Auto", NULL,
	};
	static char const *const exposure_auto[] = {
		"Auto Mode", "Manual Mode", "Shutter Priority Mode", "Aperture Priority Mode", NULL,
	};
	static char const *const colorfx[] = {
		"None",       "Black & White", "Sepia",       "Negative",  "Emboss", "Sketch",
		"Sky Blue",   "Grass Green",   "Skin Whiten", "Vivid",     "Aqua",   "Art Freeze",
		"Silhouette", "Solarization",  "Antique",     "Set Cb/Cr", NULL,
	};
	static char const *const camera_orientation[] = {
		"Front",
		"Back",
		"External",
		NULL,
	};
	static char const *const codec_bitrate_mode[] = {
		"Variable Bitrate",
		"Constant Bitrate",
		"Constant Quality",
		NULL,
	};
	static char const *const codec_header_mode[] = {
		"Separate Buffer",
		"Joined With 1st Frame",
		NULL,
	};
	static char const *const h264_level[] = {
		"1", "1b",  "1.1", "1.2", "1.3", "2",   "2.1", "2.2", "3",   "3.1", "3.2",
		"4", "4.1", "4.2", "5",   "5.1", "5.2", "6.0", "6.1", "6.2", NULL,
	};
	static char const *const h264_profile[] = {
		"Baseline",
		"Constrained Baseline",
		"Main",
		"Extended",
		"High",
		"High 10",
		"High 422",
		"High 444 Predictive",
		"High 10 Intra",
		"High 422 Intra",
		"High 444 Intra",
		"CAVLC 444 Intra",
		"Scalable Baseline",
		"Scalable High",
		"Scalable High Intra",
		"Stereo High",
		"Multiview High",
		"Constrained High",
		NULL,
	};

	switch (id) {
	/* User control menus */
	case VIDEO_CID_POWER_LINE_FREQUENCY:
		return power_line_frequency;

	/* Camera control menus */
	case VIDEO_CID_EXPOSURE_AUTO:
		return exposure_auto;
	case VIDEO_CID_COLORFX:
		return colorfx;
	case VIDEO_CID_CAMERA_ORIENTATION:
		return camera_orientation;

	/* Codec control menus */
	case VIDEO_CID_CODEC_BITRATE_MODE:
		return codec_bitrate_mode;
	case VIDEO_CID_CODEC_HEADER_MODE:
		return codec_header_mode;
	case VIDEO_CID_H264_LEVEL:
		return h264_level;
	case VIDEO_CID_H264_PROFILE:
		return h264_profile;
	default:
		return NULL;
	}
}

static inline int check_range(enum video_ctrl_type type, struct video_ctrl_range range)
{
	switch (type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		if (range.step != 1 || range.max > 1 || range.min < 0 ||
		    !IN_RANGE(range.def, range.min, range.max)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER:
		if (range.step == 0 || range.min > range.max ||
		    !IN_RANGE(range.def, range.min, range.max)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_INTEGER64:
		if (range.step64 == 0 || range.min64 > range.max64 ||
		    !IN_RANGE(range.def64, range.min64, range.max64)) {
			return -ERANGE;
		}
		return 0;
	case VIDEO_CTRL_TYPE_MENU:
	case VIDEO_CTRL_TYPE_INTEGER_MENU:
		if (!IN_RANGE(range.min, 0, range.max) ||
		    !IN_RANGE(range.def, range.min, range.max)) {
			return -ERANGE;
		}
		return 0;
	default:
		return 0;
	}
}

/* Standard controls have a fixed type and flags, private controls keep what the driver set */
static inline void set_type_flag(struct video_ctrl *ctrl)
{
	switch (ctrl->id) {
	case VIDEO_CID_AUTO_WHITE_BALANCE:
	case VIDEO_CID_AUTOGAIN:
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
	case VIDEO_CID_HUE_AUTO:
	case VIDEO_CID_AUTOBRIGHTNESS:
	case VIDEO_CID_EXPOSURE_AUTO_PRIORITY:
	case VIDEO_CID_FOCUS_AUTO:
	case VIDEO_CID_WIDE_DYNAMIC_RANGE:
	case VIDEO_CID_CODEC_FRAME_RC_ENABLE:
	case VIDEO_CID_CODEC_MB_RC_ENABLE:
	case VIDEO_CID_CODEC_PREPEND_SPSPPS_TO_IDR:
		ctrl->type = VIDEO_CTRL_TYPE_BOOLEAN;
		break;
	case VIDEO_CID_POWER_LINE_FREQUENCY:
	case VIDEO_CID_EXPOSURE_AUTO:
	case VIDEO_CID_COLORFX:
	case VIDEO_CID_TEST_PATTERN:
	case VIDEO_CID_CAMERA_ORIENTATION:
	case VIDEO_CID_CODEC_BITRATE_MODE:
	case VIDEO_CID_CODEC_HEADER_MODE:
	case VIDEO_CID_H264_LEVEL:
	case VIDEO_CID_H264_PROFILE:
		ctrl->type = VIDEO_CTRL_TYPE_MENU;
		break;
	case VIDEO_CID_CODEC_FORCE_KEY_FRAME:
		ctrl->type = VIDEO_CTRL_TYPE_BUTTON;
		break;
	case VIDEO_CID_PIXEL_RATE:
		ctrl->type = VIDEO_CTRL_TYPE_INTEGER64;
		ctrl->flags |= VIDEO_CTRL_FLAG_READ_ONLY;
		break;
	case VIDEO_CID_LINK_FREQ:
		ctrl->type = VIDEO_CTRL_TYPE_INTEGER_MENU;
		break;
	case VIDEO_CID_PAN_RELATIVE:
	case VIDEO_CID_TILT_RELATIVE:
	case VIDEO_CID_FOCUS_RELATIVE:
	case VIDEO_CID_IRIS_RELATIVE:
	case VIDEO_CID_ZOOM_RELATIVE:
		/* A relative move is an action, repeating it with the same value makes sense */
		ctrl->type = VIDEO_CTRL_TYPE_INTEGER;
		ctrl->flags |= VIDEO_CTRL_FLAG_WRITE_ONLY | VIDEO_CTRL_FLAG_EXECUTE_ON_WRITE;
		break;
	default:
		if (ctrl->id < VIDEO_CID_PRIVATE_BASE || ctrl->type == 0) {
			ctrl->type = VIDEO_CTRL_TYPE_INTEGER;
		}
		break;
	}

	if (ctrl->type == VIDEO_CTRL_TYPE_BUTTON) {
		ctrl->flags |= VIDEO_CTRL_FLAG_WRITE_ONLY | VIDEO_CTRL_FLAG_EXECUTE_ON_WRITE;
	}
}

/* The index range of a menu is fixed, its entries come from the driver or the standard menus */
static int set_menu(struct video_ctrl *ctrl)
{
	ctrl->range.min = 0;
	ctrl->range.step = 1;

	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER_MENU) {
		return ctrl->int_menu == NULL ? -EINVAL : 0;
	}

	if (ctrl->menu == NULL) {
		ctrl->menu = video_get_std_menu_ctrl(ctrl->id);
	}

	/* A menu sized by the driver may stay unnamed, otherwise its entries give the size */
	if (ctrl->range.max != 0) {
		if (ctrl->menu != NULL) {
			for (int32_t i = 0; i <= ctrl->range.max; i++) {
				if (ctrl->menu[i] == NULL) {
					return -ERANGE;
				}
			}
		}
		return 0;
	}

	if (ctrl->menu == NULL || ctrl->menu[0] == NULL) {
		return -EINVAL;
	}

	while (ctrl->menu[ctrl->range.max + 1] != NULL) {
		ctrl->range.max++;
	}

	return 0;
}

int video_register_ctrl(struct video_ctrl *ctrl, const struct device *dev)
{
	int ret;
	struct video_ctrl *vc;
	struct video_device *vdev;

	if (ctrl == NULL || dev == NULL || ctrl->id < VIDEO_CID_BASE) {
		return -EINVAL;
	}

	vdev = video_find_vdev(dev);
	if (vdev == NULL) {
		return -EINVAL;
	}

	set_type_flag(ctrl);

	switch (ctrl->type) {
	case VIDEO_CTRL_TYPE_BOOLEAN:
		ctrl->range.min = 0;
		ctrl->range.max = 1;
		ctrl->range.step = 1;
		break;
	case VIDEO_CTRL_TYPE_BUTTON:
		ctrl->range = (struct video_ctrl_range){0};
		break;
	case VIDEO_CTRL_TYPE_MENU:
	case VIDEO_CTRL_TYPE_INTEGER_MENU:
		ret = set_menu(ctrl);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		break;
	}

	ret = check_range(ctrl->type, ctrl->range);
	if (ret < 0) {
		return ret;
	}

	ctrl->vdev = vdev;

	if (ctrl->type == VIDEO_CTRL_TYPE_INTEGER64) {
		ctrl->val64 = ctrl->range.def64;
	} else {
		ctrl->val = ctrl->range.def;
	}

	/* Insert in an ascending order of ctrl's id */
	SYS_DLIST_FOR_EACH_CONTAINER(&vdev->ctrls, vc, node) {
		if (vc->id > ctrl->id) {
			sys_dlist_insert(&vc->node, &ctrl->node);
			return 0;
		}
	}

	sys_dlist_append(&vdev->ctrls, &ctrl->node);

	return 0;
}

int video_register_ctrls(struct video_ctrl *ctrls, size_t num, const struct device *dev)
{
	int ret;

	if (ctrls == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < num; i++) {
		ret = video_register_ctrl(&ctrls[i], dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int video_read_reg_retry(const struct i2c_dt_spec *i2c, uint8_t *buf_w, size_t size_w,
				uint8_t *buf_r, size_t size_r)
{
	int ret;

	for (int i = 0;; i++) {
		ret = i2c_write_read_dt(i2c, buf_w, size_w, buf_r, size_r);
		if (ret == 0) {
			break;
		}
		if (i == CONFIG_VIDEO_I2C_RETRY_NUM) {
			LOG_HEXDUMP_ERR(buf_w, size_w, "failed to write-read to I2C register");
			return ret;
		}
		if (CONFIG_VIDEO_I2C_RETRY_NUM > 0) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

int video_read_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t *reg_data)
{
	size_t addr_size = FIELD_GET(VIDEO_REG_ADDR_SIZE_MASK, reg_addr);
	size_t data_size = FIELD_GET(VIDEO_REG_DATA_SIZE_MASK, reg_addr);
	bool big_endian = FIELD_GET(VIDEO_REG_ENDIANNESS_MASK, reg_addr);
	uint16_t addr = FIELD_GET(VIDEO_REG_ADDR_MASK, reg_addr);
	uint8_t buf_w[sizeof(uint16_t)] = {0};
	uint8_t *data_ptr;
	int ret;

	if (i2c == NULL || reg_data == NULL || addr_size == 0 || data_size == 0) {
		return -EINVAL;
	}

	*reg_data = 0;

	if (big_endian) {
		/* Casting between data sizes in big-endian requires re-aligning */
		data_ptr = (uint8_t *)reg_data + sizeof(*reg_data) - data_size;
	} else {
		/* Casting between data sizes in little-endian is a no-op */
		data_ptr = (uint8_t *)reg_data;
	}

	for (int i = 0; i < data_size; i++) {
		if (addr_size == 1) {
			buf_w[0] = addr + i;
		} else {
			sys_put_be16(addr + i, &buf_w[0]);
		}

		ret = video_read_reg_retry(i2c, buf_w, addr_size, &data_ptr[i], 1);
		if (ret < 0) {
			LOG_ERR("Failed to read from register 0x%x", addr + i);
			return ret;
		}

		LOG_HEXDUMP_DBG(buf_w, addr_size, "Data written to the I2C device...");
		LOG_HEXDUMP_DBG(&data_ptr[i], 1, "... data read back from the I2C device");
	}

	*reg_data = big_endian ? sys_be32_to_cpu(*reg_data) : sys_le32_to_cpu(*reg_data);

	return 0;
}

static int video_write_reg_retry(const struct i2c_dt_spec *i2c, uint8_t *buf_w, size_t size)
{
	int ret;

	if (i2c == NULL || buf_w == NULL) {
		return -EINVAL;
	}

	for (int i = 0;; i++) {
		ret = i2c_write_dt(i2c, buf_w, size);
		if (ret == 0) {
			break;
		}
		if (i == CONFIG_VIDEO_I2C_RETRY_NUM) {
			LOG_HEXDUMP_ERR(buf_w, size, "failed to write to I2C register");
			return ret;
		}
		if (CONFIG_VIDEO_I2C_RETRY_NUM > 0) {
			k_sleep(K_MSEC(1));
		}
	}

	return 0;
}

int video_write_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t reg_data)
{
	size_t addr_size = FIELD_GET(VIDEO_REG_ADDR_SIZE_MASK, reg_addr);
	size_t data_size = FIELD_GET(VIDEO_REG_DATA_SIZE_MASK, reg_addr);
	bool big_endian = FIELD_GET(VIDEO_REG_ENDIANNESS_MASK, reg_addr);
	uint16_t addr = FIELD_GET(VIDEO_REG_ADDR_MASK, reg_addr);
	uint8_t buf_w[sizeof(uint16_t) + sizeof(uint32_t)] = {0};
	uint8_t *data_ptr;
	int ret;

	if (i2c == NULL || addr_size == 0 || data_size == 0) {
		return -EINVAL;
	}

	if (big_endian) {
		/* Casting between data sizes in big-endian requires re-aligning */
		reg_data = sys_cpu_to_be32(reg_data);
		data_ptr = (uint8_t *)&reg_data + sizeof(reg_data) - data_size;
	} else {
		/* Casting between data sizes in little-endian is a no-op */
		reg_data = sys_cpu_to_le32(reg_data);
		data_ptr = (uint8_t *)&reg_data;
	}

	for (int i = 0; i < data_size; i++) {
		/* The address is always big-endian as per CCI standard */
		if (addr_size == 1) {
			buf_w[0] = addr + i;
		} else {
			sys_put_be16(addr + i, &buf_w[0]);
		}

		buf_w[addr_size] = data_ptr[i];

		LOG_HEXDUMP_DBG(buf_w, addr_size + 1, "Data written to the I2C device");

		ret = video_write_reg_retry(i2c, buf_w, addr_size + 1);
		if (ret < 0) {
			LOG_ERR("Failed to write to register 0x%x", addr + i);
			return ret;
		}
	}

	return 0;
}

int video_modify_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t field_mask,
			 uint32_t field_value)
{
	uint32_t reg;
	int ret;

	ret = video_read_cci_reg(i2c, reg_addr, &reg);
	if (ret < 0) {
		return ret;
	}

	return video_write_cci_reg(i2c, reg_addr, (reg & ~field_mask) | field_value);
}

int video_write_cci_multiregs(const struct i2c_dt_spec *i2c, const struct video_reg *regs,
			      size_t num_regs)
{
	int ret;

	if (regs == NULL) {
		return -EINVAL;
	}

	for (int i = 0; i < num_regs; i++) {
		ret = video_write_cci_reg(i2c, regs[i].addr, regs[i].data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int video_write_cci_multiregs8(const struct i2c_dt_spec *i2c, const struct video_reg8 *regs,
			       size_t num_regs)
{
	int ret;

	if (regs == NULL) {
		return -EINVAL;
	}

	for (int i = 0; i < num_regs; i++) {
		ret = video_write_cci_reg(i2c, regs[i].addr | VIDEO_REG_ADDR8_DATA8, regs[i].data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int video_write_cci_multiregs16(const struct i2c_dt_spec *i2c, const struct video_reg16 *regs,
				size_t num_regs)
{
	int ret;

	if (regs == NULL) {
		return -EINVAL;
	}

	for (int i = 0; i < num_regs; i++) {
		ret = video_write_cci_reg(i2c, regs[i].addr | VIDEO_REG_ADDR16_DATA8, regs[i].data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
