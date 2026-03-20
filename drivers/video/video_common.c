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

int video_init_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
		    struct video_ctrl_range range)
{
	int ret;
	uint32_t flags;
	enum video_ctrl_type type;
	struct video_ctrl *vc;
	struct video_device *vdev;

	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(ctrl != NULL);

	vdev = video_find_vdev(dev);
	if (!vdev) {
		return -EINVAL;
	}

	/* Sanity checks */
	if (id < VIDEO_CID_BASE) {
		return -EINVAL;
	}

	set_type_flag(id, &type, &flags);

	ret = check_range(type, range);
	if (ret) {
		return ret;
	}

	ctrl->cluster_sz = 0;
	ctrl->cluster = NULL;
	ctrl->is_auto = false;
	ctrl->has_volatiles = false;
	ctrl->menu = NULL;
	ctrl->vdev = vdev;
	ctrl->id = id;
	ctrl->type = type;
	ctrl->flags = flags;
	ctrl->range = range;

	if (type == VIDEO_CTRL_TYPE_INTEGER64) {
		ctrl->val64 = range.def64;
	} else {
		ctrl->val = range.def;
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

int video_init_menu_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
			 uint8_t def, const char *const menu[])
{
	int ret;
	uint8_t sz = 0;
	const char *const *_menu = menu ? menu : video_get_std_menu_ctrl(id);

	if (!_menu) {
		return -EINVAL;
	}

	while (_menu[sz]) {
		sz++;
	}

	ret = video_init_ctrl(
		ctrl, dev, id,
		(struct video_ctrl_range){.min = 0, .max = sz - 1, .step = 1, .def = def});

	if (ret) {
		return ret;
	}

	ctrl->menu = _menu;

	return 0;
}

int video_init_int_menu_ctrl(struct video_ctrl *ctrl, const struct device *dev, uint32_t id,
			     uint8_t def, const int64_t menu[], size_t menu_len)
{
	int ret;

	if (!menu) {
		return -EINVAL;
	}

	ret = video_init_ctrl(
		ctrl, dev, id,
		(struct video_ctrl_range){.min = 0, .max = menu_len - 1, .step = 1, .def = def});

	if (ret) {
		return ret;
	}

	ctrl->int_menu = menu;

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

	__ASSERT_NO_MSG(i2c != NULL);
	__ASSERT_NO_MSG(reg_data != NULL);
	__ASSERT(addr_size > 0, "The address must have a address size flag");
	__ASSERT(data_size > 0, "The address must have a data size flag");

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

	__ASSERT_NO_MSG(i2c != NULL);
	__ASSERT_NO_MSG(buf_w != NULL);

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

	__ASSERT_NO_MSG(i2c != NULL);
	__ASSERT(addr_size > 0, "The address must have a address size flag");
	__ASSERT(data_size > 0, "The address must have a data size flag");

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

	__ASSERT_NO_MSG(regs != NULL);

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

	__ASSERT_NO_MSG(regs != NULL);

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

	__ASSERT_NO_MSG(regs != NULL);

	for (int i = 0; i < num_regs; i++) {
		ret = video_write_cci_reg(i2c, regs[i].addr | VIDEO_REG_ADDR16_DATA8, regs[i].data);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
