/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2024-2025, tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_common, CONFIG_VIDEO_LOG_LEVEL);

#if defined(CONFIG_VIDEO_BUFFER_USE_SHARED_MULTI_HEAP)
#include <zephyr/multi_heap/shared_multi_heap.h>

#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	shared_multi_heap_aligned_alloc(CONFIG_VIDEO_BUFFER_SMH_ATTRIBUTE, align, size)
#define VIDEO_COMMON_FREE(block) shared_multi_heap_free(block)
#else
K_HEAP_DEFINE(video_buffer_pool, CONFIG_VIDEO_BUFFER_POOL_SZ_MAX*CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
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
	uint64_t min = stepwise->min.numerator;
	uint64_t max = stepwise->max.numerator;
	uint64_t step = stepwise->step.numerator;
	uint64_t goal = desired->numerator;

	/* Set a common denominator to all values */
	min *= stepwise->max.denominator * stepwise->step.denominator * desired->denominator;
	max *= stepwise->min.denominator * stepwise->step.denominator * desired->denominator;
	step *= stepwise->min.denominator * stepwise->max.denominator * desired->denominator;
	goal *= stepwise->min.denominator * stepwise->max.denominator * stepwise->step.denominator;

	/* Saturate the desired value to the min/max supported */
	goal = CLAMP(goal, min, max);

	/* Compute a numerator and denominator */
	match->numerator = min + DIV_ROUND_CLOSEST(goal - min, step) * step;
	match->denominator = stepwise->min.denominator * stepwise->max.denominator *
			     stepwise->step.denominator * desired->denominator;
}

void video_closest_frmival(const struct device *dev, enum video_endpoint_id ep,
			   struct video_frmival_enum *match)
{
	uint64_t best_diff_nsec = INT32_MAX;
	struct video_frmival desired = match->discrete;
	struct video_frmival_enum fie = {.format = match->format};

	__ASSERT(match->type != VIDEO_FRMIVAL_TYPE_STEPWISE,
		 "cannot find range matching the range, only a value matching the range");

	for (fie.index = 0; video_enum_frmival(dev, ep, &fie) == 0; fie.index++) {
		struct video_frmival tmp = {0};
		uint64_t diff_nsec = 0, a, b;

		switch (fie.type) {
		case VIDEO_FRMIVAL_TYPE_DISCRETE:
			tmp = fie.discrete;
			break;
		case VIDEO_FRMIVAL_TYPE_STEPWISE:
			video_closest_frmival_stepwise(&fie.stepwise, &desired, &tmp);
			break;
		default:
			__ASSERT(false, "invalid answer from the queried video device");
		}

		a = video_frmival_nsec(&desired);
		b = video_frmival_nsec(&tmp);
		diff_nsec = a > b ? a - b : b - a;
		if (diff_nsec < best_diff_nsec) {
			best_diff_nsec = diff_nsec;
			match->index = fie.index;
			memcpy(&match->discrete, &tmp, sizeof(tmp));
		}
	}
}

int video_read_cci_reg(const struct i2c_dt_spec *i2c, uint32_t flags, uint32_t *data)
{
	size_t addr_size = FIELD_GET(VIDEO_REG_ADDR_SIZE_MASK, flags);
	size_t data_size = FIELD_GET(VIDEO_REG_DATA_SIZE_MASK, flags);
	bool big_endian = FIELD_GET(VIDEO_REG_ENDIANNESS_MASK, flags);
	uint16_t addr = FIELD_GET(VIDEO_REG_ADDR_MASK, flags);
	uint8_t buf_w[sizeof(uint16_t)] = {0};
	uint8_t *data_ptr;
	int ret;

	if (big_endian) {
		/* Casting between data sizes in big-endian requires re-aligning */
		*data = 0;
		data_ptr = (uint8_t *)data + sizeof(data) - data_size;
	} else {
		/* Casting between data sizes in little-endian is a no-op */
		*data = 0;
		data_ptr = (uint8_t *)data;
	}

	for (int i = 0; i < data_size; i++) {
		if (addr_size == 1) {
			buf_w[0] = addr + i;
		} else {
			sys_put_be16(addr + i, &buf_w[0]);
		}

		ret = i2c_write_read_dt(i2c, buf_w, addr_size, &data_ptr[i], 1);
		if (ret != 0) {
			LOG_ERR("Failed to read from register 0x%x", addr + i);
			return ret;
		}

		LOG_HEXDUMP_DBG(buf_w, addr_size, "Data written to the I2C device...");
		LOG_HEXDUMP_DBG(&data_ptr[i], 1, "... data read back from the I2C device");
	}

	*data = big_endian ? sys_be32_to_cpu(*data) : sys_le32_to_cpu(*data);

	return 0;
}

static int video_write_reg_retry(const struct i2c_dt_spec *i2c, uint8_t *buf_w, size_t size)
{
	int ret = 0;

	for (int i = 0; i < CONFIG_VIDEO_I2C_RETRY_NUM; i++) {
		ret = i2c_write_dt(i2c, buf_w, size);
		if (ret == 0) {
			return 0;
		}

		k_sleep(K_MSEC(1));
	}

	LOG_HEXDUMP_ERR(buf_w, size, "failed to write register configuration over I2C");

	return ret;
}

int video_write_cci_reg(const struct i2c_dt_spec *i2c, uint32_t flags, uint32_t data)
{
	size_t addr_size = FIELD_GET(VIDEO_REG_ADDR_SIZE_MASK, flags);
	size_t data_size = FIELD_GET(VIDEO_REG_DATA_SIZE_MASK, flags);
	bool big_endian = FIELD_GET(VIDEO_REG_ENDIANNESS_MASK, flags);
	uint16_t addr = FIELD_GET(VIDEO_REG_ADDR_MASK, flags);
	uint8_t buf_w[sizeof(uint16_t) + sizeof(uint32_t)] = {0};
	uint8_t *data_ptr;
	int ret;

	if (big_endian) {
		/* Casting between data sizes in big-endian requires re-aligning */
		data = sys_cpu_to_be32(data);
		data_ptr = (uint8_t *)&data + sizeof(data) - data_size;
	} else {
		/* Casting between data sizes in little-endian is a no-op */
		data = sys_cpu_to_le32(data);
		data_ptr = (uint8_t *)&data;
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
		if (ret != 0) {
			LOG_ERR("Failed to write to register 0x%x", addr + i);
			return ret;
		}
	}

	return 0;
}

int video_write_cci_field(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t field_mask,
			  uint32_t field_value)
{
	uint32_t reg;
	int ret;

	ret = video_read_cci_reg(i2c, reg_addr, &reg);
	if (ret != 0) {
		return ret;
	}

	return video_write_cci_reg(i2c, reg_addr, (reg & ~field_mask) | field_value);
}

int video_write_cci_multi(const struct i2c_dt_spec *i2c, const struct video_reg *regs)
{
	int ret;

	for (int i = 0; regs[i].addr != 0; i++) {
		ret = video_write_cci_reg(i2c, regs[i].addr, regs[i].data);
		if (ret != 0) {
			LOG_ERR("Failed to write 0x%04x to register 0x%02x",
				regs[i].data, regs[i].addr);
			return ret;
		}
	}

	return 0;
}

/* Common implementation for imagers (a.k.a. image sensor) drivers */

int video_imager_set_mode(const struct device *dev, const struct video_imager_mode *mode)
{
	const struct video_imager_config *cfg = dev->config;
	struct video_imager_data *data = dev->data;
	int ret;

	if (data->mode == mode) {
		LOG_DBG("%s is arlready in the mode requested", dev->name);
		return 0;
	}

	/* Write each register table to the device */
	for (int i = 0; i < ARRAY_SIZE(mode->regs) && mode->regs[i] != NULL; i++) {
		ret = cfg->write_multi(&cfg->i2c, mode->regs[i]);
		if (ret != 0) {
			LOG_ERR("Could not set %s to mode %p, %u FPS", dev->name, mode, mode->fps);
			return ret;
		}
	}

	data->mode = mode;

	return 0;
}

int video_imager_set_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival)
{
	const struct video_imager_config *cfg = dev->config;
	struct video_imager_data *data = dev->data;
	struct video_frmival_enum fie = {.format = &data->fmt, .discrete = *frmival};

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	video_closest_frmival(dev, ep, &fie);

	return video_imager_set_mode(dev, &cfg->modes[data->fmt_id][fie.index]);
}

int video_imager_get_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival)
{
	struct video_imager_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	frmival->numerator = 1;
	frmival->denominator = data->mode->fps;

	return 0;
}

int video_imager_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
			      struct video_frmival_enum *fie)
{
	const struct video_imager_config *cfg = dev->config;
	const struct video_imager_mode *modes;
	size_t fmt_id = 0;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	ret = video_format_caps_index(cfg->fmts, fie->format, &fmt_id);
	if (ret != 0) {
		LOG_ERR("Format '%s' %ux%u not found for %s",
			VIDEO_FOURCC_TO_STR(fie->format->pixelformat),
			fie->format->width, fie->format->height, dev->name);
		return ret;
	}

	modes = cfg->modes[fmt_id];

	for (int i = 0;; i++) {
		if (modes[i].fps == 0) {
			return -EINVAL;
		}

		if (i == fie->index) {
			fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
			fie->discrete.numerator = 1;
			fie->discrete.denominator = modes[i].fps;
			break;
		}
	}

	return 0;
}

int video_imager_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			 struct video_format *fmt)
{
	const struct video_imager_config *cfg = dev->config;
	struct video_imager_data *data = dev->data;
	size_t fmt_id;
	int ret;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		LOG_ERR("Only the output endpoint is supported for %s", dev->name);
		return -EINVAL;
	}

	ret = video_format_caps_index(cfg->fmts, fmt, &fmt_id);
	if (ret != 0) {
		LOG_ERR("Format '%s' %ux%u not found for device %s",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height, dev->name);
		return ret;
	}

	ret = video_imager_set_mode(dev, &cfg->modes[fmt_id][0]);
	if (ret != 0) {
		return ret;
	}

	data->fmt_id = fmt_id;
	data->fmt = *fmt;

	return 0;
}

int video_imager_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			 struct video_format *fmt)
{
	struct video_imager_data *data = dev->data;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	*fmt = data->fmt;

	return 0;
}

int video_imager_get_caps(const struct device *dev, enum video_endpoint_id ep,
			  struct video_caps *caps)
{
	const struct video_imager_config *cfg = dev->config;

	if (ep != VIDEO_EP_OUT && ep != VIDEO_EP_ALL) {
		return -EINVAL;
	}

	caps->format_caps = cfg->fmts;

	return 0;
}

int video_imager_init(const struct device *dev, const struct video_reg *init_regs,
		      int default_fmt_idx)
{
	const struct video_imager_config *cfg = dev->config;
	struct video_format fmt;
	int ret;

	__ASSERT_NO_MSG(cfg->modes != NULL);
	__ASSERT_NO_MSG(cfg->fmts != NULL);

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus device %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	if (init_regs != NULL) {
		ret = cfg->write_multi(&cfg->i2c, init_regs);
		if (ret != 0) {
			LOG_ERR("Could not set %s initial registers", dev->name);
			return ret;
		}
	}

	fmt.pixelformat = cfg->fmts[default_fmt_idx].pixelformat;
	fmt.width = cfg->fmts[default_fmt_idx].width_max;
	fmt.height = cfg->fmts[default_fmt_idx].height_max;
	fmt.pitch = fmt.width * video_bits_per_pixel(fmt.pixelformat) / BITS_PER_BYTE;

	ret = video_set_format(dev, VIDEO_EP_OUT, &fmt);
	if (ret != 0) {
		LOG_ERR("Failed to set %s to default format '%s' %ux%u",
			dev->name, VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);
		return ret;
	}

	return 0;
}
