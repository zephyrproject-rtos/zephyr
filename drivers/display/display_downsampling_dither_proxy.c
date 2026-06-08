/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT zephyr_downsampling_dither_proxy

LOG_MODULE_REGISTER(downsampling_dither, CONFIG_DISPLAY_LOG_LEVEL);

#define DITHER_L_8_THRES	0xff
#define MED_FORWARD		3
#define MED_BUF_SIZE		((8 + MED_FORWARD) * MED_FORWARD)

enum downsampling_dither_algorithm {
	DOWNSAMPLING_DITHER_ALGO_NONE,
	DOWNSAMPLING_DITHER_ALGO_MINIMAL_ERROR,
	DOWNSAMPLING_DITHER_ALGO_MINIMAL_ERROR_DISTRIBUTED,
	DOWNSAMPLING_DITHER_ALGO_MAX
};

struct downsampling_dither_config {
	const struct device *display_device;
	enum display_pixel_format pixel_format;
	enum display_pixel_format target_pixel_format;
	enum downsampling_dither_algorithm algorithm;
	uint16_t width;
	uint16_t height;
	uint16_t width_ratio;
	uint16_t height_ratio;
	uint8_t *buf;
	size_t buf_size;
};

struct downsampling_dither_algo_data {
	int16_t error[MED_BUF_SIZE];
};

struct downsampling_dither_data {
	struct downsampling_dither_algo_data algo_data;
};

typedef int (*downsampling_conversion_func)(const uint8_t *bufi, uint8_t *bufo,
					    const struct display_buffer_descriptor *desc_i,
					    struct display_buffer_descriptor *desc_o,
					    uint16_t width_ratio, uint16_t height_ratio,
					    struct downsampling_dither_algo_data *data);

static int downsampling_dither_blanking_on(const struct device *dev)
{
	const struct downsampling_dither_config *config = dev->config;

	return display_blanking_on(config->display_device);
}

static int downsampling_dither_blanking_off(const struct device *dev)
{
	const struct downsampling_dither_config *config = dev->config;

	return display_blanking_off(config->display_device);
}

static int downsampling_dither_set_contrast(const struct device *dev, uint8_t contrast)
{
	const struct downsampling_dither_config *config = dev->config;

	return display_set_contrast(config->display_device, contrast);
}

static void downsampling_dither_get_capabilities(const struct device *dev,
						struct display_capabilities *caps)
{
	const struct downsampling_dither_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = config->pixel_format;
	caps->current_pixel_format = config->pixel_format;

}

static int downsampling_dither_convert_l_8_mono_vtiled_none(const uint8_t *bufi, uint8_t *bufo,
						const struct display_buffer_descriptor *desc_i,
						struct display_buffer_descriptor *desc_o,
						uint16_t width_ratio, uint16_t height_ratio,
						struct downsampling_dither_algo_data *data)
{
	if (desc_o->height % 8 && desc_o->height > 8) {
		LOG_WRN("Height is not a multiple of 8, clamping");
		desc_o->height &= ~7;
		desc_o->buf_size = (desc_o->width * desc_o->height) / 8;
	} else if (desc_o->height % 8) {
		LOG_WRN("Height is not a multiple of 8, dumping frame");
		return EINVAL;
	}

	for (size_t j = 0; j < desc_o->height; j += 8 / height_ratio) {
		for (size_t i = 0; i < desc_o->width; i++) {
			bufo[(j / 8) * desc_o->width + i] = ((bufi[j / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 7)
				| ((bufi[(j + 1) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 6)
				| ((bufi[(j + 2) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 5)
				| ((bufi[(j + 3) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 4)
				| ((bufi[(j + 4) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 3)
				| ((bufi[(j + 5) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 2)
				| ((bufi[(j + 6) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 1)
				| ((bufi[(j + 7) / height_ratio * desc_i->width + i / width_ratio] & 0x80) >> 0);
		}
	}

	return 0;
}

static int downsampling_dither_convert_l_8_mono_vtiled_minimal_error(
						const uint8_t *bufi,uint8_t *bufo,
						const struct display_buffer_descriptor *desc_i,
						struct display_buffer_descriptor *desc_o,
						uint16_t width_ratio, uint16_t height_ratio,
						struct downsampling_dither_algo_data *data)
{
	int16_t error = data->error[0];
	uint8_t lbyte;

	if (desc_o->height % 8 && desc_o->height > 8) {
		LOG_WRN("Height is not a multiple of 8, clamping");
		desc_o->height &= ~7;
		desc_o->buf_size = (desc_o->width * desc_o->height) / 8;
	} else if (desc_o->height % 8) {
		LOG_WRN("Height is not a multiple of 8, dumping frame");
		return EINVAL;
	}

	for (size_t j = 0; j < desc_o->height; j += 8) {
		for (size_t i = 0; i < desc_o->width; i++) {
			lbyte = 0;
			for (size_t k = 0; k < 8; k++) {
				error += bufi[(j + k) / height_ratio
						    * desc_i->width + i / width_ratio];
				if (error >= DITHER_L_8_THRES) {
					lbyte |= (1U << k);
					error -= DITHER_L_8_THRES;
				}
			}
			bufo[(j / 8) * desc_o->width + i] = lbyte;
		}
	}

	data->error[0] = error;

	return 0;
}

static int downsampling_dither_convert_l_8_mono_vtiled_minimal_error_distributed(
						const uint8_t *bufi,uint8_t *bufo,
						const struct display_buffer_descriptor *desc_i,
						struct display_buffer_descriptor *desc_o,
						uint16_t width_ratio, uint16_t height_ratio,
						struct downsampling_dither_algo_data *data)
{
	int clerror = 0;
	int lerror;
	uint8_t lbyte;

	if (desc_o->height % 8 && desc_o->height > 8) {
		LOG_WRN("Height is not a multiple of 8, clamping");
		desc_o->height &= ~7;
		desc_o->buf_size = (desc_o->width * desc_o->height) / 8;
	} else if (desc_o->height % 8) {
		LOG_WRN("Height is not a multiple of 8, dumping frame");
		return EINVAL;
	}

	for (size_t j = 0; j < desc_o->height; j += 8) {
		memset(data->error, 0, MED_BUF_SIZE * sizeof(uint16_t));
		for (size_t i = 0; i < desc_o->width; i++) {
			lbyte = 0;
			for (size_t k = 0; k < 8; k++) {
				lerror = bufi[(j + k) / height_ratio
						* desc_i->width + i / width_ratio];
				clerror += lerror;
				data->error[k] += clerror;
				if (data->error[k] >= 0x80) {
					lbyte |= (1U << k);
					clerror = clerror / 8 - 0x100;
				}
				data->error[k * MED_FORWARD + 1] += clerror / 8;
				data->error[k * MED_FORWARD + 2] += clerror / 8;
				data->error[(k + 1) * MED_FORWARD] += clerror / 8;
				data->error[(k + 1) * MED_FORWARD + 1] += clerror / 8;
				data->error[(k + 1) * MED_FORWARD + 2] += clerror / 16;
				data->error[(k + 2) * MED_FORWARD] += clerror / 8;
			}
			for (size_t k = 0; k < 8; k++) {
				data->error[k * MED_FORWARD + 0] = data->error[k * MED_FORWARD + 1];
				data->error[k * MED_FORWARD + 1] = data->error[k * MED_FORWARD + 2];
				data->error[k * MED_FORWARD + 2] = 0;
			}
			bufo[(j / 8) * desc_o->width + i] = lbyte;
		}
		// data->error[0] = data->error[8 * MED_FORWARD];
		// data->error[1] = data->error[8 * MED_FORWARD + 1];
		// data->error[2] = data->error[8 * MED_FORWARD + 2];
		// data->error[3] = data->error[9 * MED_FORWARD];
		// data->error[4] = data->error[9 * MED_FORWARD + 1];
		// data->error[5] = 0;
	}

	return 0;
}

static const downsampling_conversion_func conversion_funcs[32][32][DOWNSAMPLING_DITHER_ALGO_MAX] = {
	/* L8 to MONO01 */
	[7][2] = {
		downsampling_dither_convert_l_8_mono_vtiled_none,
		downsampling_dither_convert_l_8_mono_vtiled_minimal_error,
		downsampling_dither_convert_l_8_mono_vtiled_minimal_error_distributed,
	},
	/* L8 to MONO10 */
	[7][3] = {
		downsampling_dither_convert_l_8_mono_vtiled_none,
		downsampling_dither_convert_l_8_mono_vtiled_minimal_error,
		downsampling_dither_convert_l_8_mono_vtiled_minimal_error_distributed,
	},
};

static int downsampling_dither_write(const struct device *dev, const uint16_t x, const uint16_t y,
				    const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct downsampling_dither_config *config = dev->config;
	struct downsampling_dither_data *data = dev->data;
	struct display_capabilities device_caps;
	struct display_buffer_descriptor desc_target = {
		.width = desc->width * config->width_ratio,
		.height = desc->height * config->height_ratio,
		.pitch = desc->pitch * config->width_ratio,
		.frame_incomplete = desc->frame_incomplete,
	};
	uint16_t x_target = x * config->width_ratio;
	uint16_t y_target = y * config->height_ratio;
	uint8_t format_i, format_o;
	int ret;

	switch (config->target_pixel_format) {
	case PIXEL_FORMAT_MONO01:
	case PIXEL_FORMAT_MONO10:
		desc_target.buf_size = (desc_target.width * desc_target.height) / 8;
		break;
	default:
		LOG_ERR("Pixel format is invalid");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_ERR("Display buffer is not available");
		return -ENOBUFS;
	}

	if (config->buf_size < desc_target.buf_size) {
		LOG_ERR("Buffer is too small");
		return -ENOBUFS;
	}

	display_get_capabilities(config->display_device, &device_caps);

	format_i = find_lsb_set(config->pixel_format);
	format_o = find_lsb_set(config->target_pixel_format);

	// LOG_ERR("[%d] = %p, [%d][%d] = %p, [%d][%d][%d] = %p %p", format_i, conversion_funcs[format_i],
	// 	format_i, format_o, conversion_funcs[format_i][format_o],
	// format_i, format_o, config->algorithm, conversion_funcs[format_i][format_o][config->algorithm],
	// 	downsampling_dither_convert_l_8_mono_vtiled_none);
 //
	// LOG_ERR("ratio: %u %u", config->width_ratio, config->height_ratio);
	// LOG_ERR("source x:%d, y:%d, w:%d, h:%d", x, y, desc->width, desc->height);
	// LOG_ERR("target x:%d, y:%d, w:%d, h:%d", x_target, y_target, desc_target.width, desc_target.height);

	if (conversion_funcs[format_i][format_o][config->algorithm] == NULL) {
		return -ENOTSUP;
	}

	ret = conversion_funcs[format_i][format_o][config->algorithm](
		buf, config->buf, desc, &desc_target, config->width_ratio, config->height_ratio,
		&data->algo_data);
	if (ret < 0) {
		LOG_ERR("Failed to convert data");
		return -EIO;
	} else if (ret > 0) {
		return 0;
	}

	return display_write(config->display_device, x_target, y_target, &desc_target, config->buf);
}

static DEVICE_API(display, downsampling_dither_driver_api) = {
	.blanking_on = downsampling_dither_blanking_on,
	.blanking_off = downsampling_dither_blanking_off,
	.write = downsampling_dither_write,
	.set_contrast = downsampling_dither_set_contrast,
	.get_capabilities = downsampling_dither_get_capabilities,
};

static int downsampling_dither_init(const struct device *dev)
{
	return 0;
}

#define DOWNSAMPLING_DITHER_PROXY_DEFINE(node_id)                                                               \
	static uint8_t buf##node_id[DT_PROP(node_id, height) * DT_PROP(node_id, width)];                   \
	static struct downsampling_dither_data data##node_id = {                   \
		.algo_data.error = 0,                   \
	};                   \
	static const struct downsampling_dither_config config##node_id = {                                     \
		.display_device = DEVICE_DT_GET(DT_PROP(node_id, display_device)),                                     \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.pixel_format = DT_PROP(node_id, pixel_format),                              \
		.target_pixel_format = DT_PROP(node_id, target_pixel_format),                \
		.algorithm = DT_ENUM_IDX(node_id, algorithm),                                \
		.width_ratio = DT_PROP(DT_PROP(node_id, display_device), width) \
		/ DT_PROP(node_id, width),                         \
		.height_ratio = DT_PROP(DT_PROP(node_id, display_device), height) \
		/ DT_PROP(node_id, height),                         \
		.buf = buf##node_id,                   \
		.buf_size = sizeof(buf##node_id),                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, downsampling_dither_init, NULL, &data##node_id, &config##node_id,            \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &downsampling_dither_driver_api);

DT_FOREACH_STATUS_OKAY(zephyr_downsampling_dither_proxy, DOWNSAMPLING_DITHER_PROXY_DEFINE)
