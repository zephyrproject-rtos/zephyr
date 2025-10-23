/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024-2025, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_common, CONFIG_VIDEO_LOG_LEVEL);

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else
K_HEAP_DEFINE(video_buffer_pool,
		CONFIG_VIDEO_BUFFER_POOL_SZ_MAX * CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	k_heap_aligned_alloc(&video_buffer_pool, align, size, timeout);
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)
#endif

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct mem_block {
	void *data;
};

static struct mem_block video_block[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct video_buffer *video_buffer_aligned_alloc(size_t size, size_t align, k_timeout_t timeout)
{
	struct video_buffer *vbuf = NULL;
	struct mem_block *block;
	int i;

	/* find available video buffer */
	for (i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			vbuf = &video_buf[i];
			block = &video_block[i];
			break;
		}
	}

	if (vbuf == NULL) {
		return NULL;
	}

	/* Alloc buffer memory */
	block->data = VIDEO_COMMON_HEAP_ALLOC(align, size, timeout);
	if (block->data == NULL) {
		return NULL;
	}

	vbuf->buffer = block->data;
	vbuf->size = size;
	vbuf->bytesused = 0;

	return vbuf;
}

struct video_buffer *video_buffer_alloc(size_t size, k_timeout_t timeout)
{
	return video_buffer_aligned_alloc(size, sizeof(void *), timeout);
}

void video_buffer_release(struct video_buffer *vbuf)
{
	struct mem_block *block = NULL;
	int i;

	__ASSERT_NO_MSG(vbuf != NULL);

	/* vbuf to block */
	for (i = 0; i < ARRAY_SIZE(video_block); i++) {
		if (video_block[i].data == vbuf->buffer) {
			block = &video_block[i];
			break;
		}
	}

	vbuf->buffer = NULL;
	if (block) {
		VIDEO_COMMON_FREE(block->data);
	}
}

int video_format_caps_index(const struct video_format_cap *fmts, const struct video_format *fmt,
			    size_t *idx)
{
	__ASSERT_NO_MSG(fmts != NULL);
	__ASSERT_NO_MSG(fmt != NULL);
	__ASSERT_NO_MSG(idx != NULL);

	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (fmts[i].pixelformat == fmt->pixelformat &&
		    IN_RANGE(fmt->width, fmts[i].width_min, fmts[i].width_max) &&
		    IN_RANGE(fmt->height, fmts[i].height_min, fmts[i].height_max)) {
			*idx = i;
			return 0;
		}
	}
	return -ENOENT;
}

void video_closest_frmival_stepwise(const struct video_frmival_stepwise *stepwise,
				    const struct video_frmival *desired,
				    struct video_frmival *match)
{
	__ASSERT_NO_MSG(stepwise != NULL);
	__ASSERT_NO_MSG(desired != NULL);
	__ASSERT_NO_MSG(match != NULL);

	uint64_t min = stepwise->min.numerator;
	uint64_t max = stepwise->max.numerator;
	uint64_t step = stepwise->step.numerator;
	uint64_t goal = desired->numerator;

	/* Set a common denominator to all values */
	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	__ASSERT_NO_MSG(step != 0U);
	/* Prevent division by zero */
	if (step == 0U) {
		return;
	}
	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(goal, min, max);

	/* Compute a numerator and denominator */
	match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
	match->denominator = stepwise->min.denominator * stepwise->max.denominator *
			     stepwise->step.denominator * desired->denominator;
}

void video_closest_frmival(const struct device *dev, struct video_frmival_enum *match)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG(match != NULL);

	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};
	uint64_t best_diff_nsec = INT32_MAX;
	uint64_t goal_nsec = video_frmival_nsec(&desired);

	__ASSERT(match->type != VIDEO_FRMIVAL_TYPE_STEPWISE,
		 "cannot find range matching the range, only a value matching the range");

	for (fie.index = 0; video_enum_frmival(dev, &fie) == 0; fie.index++) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0;
		uint64_t tmp_nsec;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			video_closest_frmival_stepwise(&fie.stepwise, &desired, &tmp);
			break;
		default:
			CODE_UNREACHABLE;
		}

		tmp_nsec = video_frmival_nsec(&tmp);
		diff_nsec = tmp_nsec > goal_nsec ? tmp_nsec - goal_nsec : goal_nsec - tmp_nsec;

		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			match->index = fie.index;
			match->discrete = tmp;
		}

		if (diff_nsec == 0) {
			/* Exact match, stop searching a better match */
			break;
		}
	}
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

int64_t video_get_csi_link_freq(const struct device *dev, uint8_t bpp, uint8_t lane_nb)
{
	struct video_control ctrl = {
		.id = VIDEO_CID_LINK_FREQ,
	};
	struct video_ctrl_query ctrl_query = {
		.dev = dev,
		.id = VIDEO_CID_LINK_FREQ,
	};
	int ret;

	/* Try to get the LINK_FREQ value from the source device */
	ret = video_get_ctrl(dev, &ctrl);
	if (ret < 0) {
		goto fallback;
	}

	ret = video_query_ctrl(&ctrl_query);
	if (ret < 0) {
		return ret;
	}

	if (!IN_RANGE(ctrl.val, ctrl_query.range.min, ctrl_query.range.max)) {
		return -ERANGE;
	}

	if (ctrl_query.int_menu == NULL) {
		return -EINVAL;
	}

	return (int64_t)ctrl_query.int_menu[ctrl.val];

fallback:
	/* If VIDEO_CID_LINK_FREQ is not available, approximate from VIDEO_CID_PIXEL_RATE */
	ctrl.id = VIDEO_CID_PIXEL_RATE;
	ret = video_get_ctrl(dev, &ctrl);
	if (ret < 0) {
		return ret;
	}

	/* CSI D-PHY is using a DDR data bus so bitrate is twice the frequency */
	return ctrl.val64 * bpp / (2 * lane_nb);
}

int video_estimate_fmt_size(struct video_format *fmt)
{
	if (fmt == NULL) {
		return -EINVAL;
	}

	switch (fmt->pixelformat) {
	case VIDEO_PIX_FMT_JPEG:
		/* Rough estimate for the worst case (quality = 100) */
		fmt->pitch = 0;
		fmt->size = fmt->width * fmt->height * 2;
		break;
	default:
		/* Uncompressed format */
		fmt->pitch = fmt->width * video_bits_per_pixel(fmt->pixelformat) / BITS_PER_BYTE;
		if (fmt->pitch == 0) {
			return -ENOTSUP;
		}
		fmt->size = fmt->pitch * fmt->height;
		break;
	}

	return 0;
}

int video_set_compose_format(const struct device *dev, struct video_format *fmt)
{
	struct video_selection sel = {
		.type = fmt->type,
		.target = VIDEO_SEL_TGT_COMPOSE,
		.rect.left = 0,
		.rect.top = 0,
		.rect.width = fmt->width,
		.rect.height = fmt->height,
	};
	int ret;

	ret = video_set_selection(dev, &sel);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Unable to set selection compose");
		return ret;
	}

	return video_set_format(dev, fmt);
}
