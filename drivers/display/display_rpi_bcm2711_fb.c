/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_bcm2711_framebuffer

#include "display_framebuffer.h"
#include <rpi_fw.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rpi_bcm2711_fb, CONFIG_DISPLAY_LOG_LEVEL);

#define RPI_BCM2711_FB_PIXEL_ORDER_BGR 0
#define RPI_BCM2711_FB_PIXEL_ORDER_RGB 1
#define RPI_BCM2711_FB_BYTES_PER_PIXEL 4
#define RPI_BCM2711_FB_TAG_SIZE        6

#define BCM2711_PHYS_ADDR_MASK(x) (x & 0x3FFFFFFF)

struct rpi_bcm2711_fb_size {
	uint32_t width;
	uint32_t height;
};

struct rpi_bcm2711_fb_alloc {
	uint32_t buffer;
	uint32_t size;
};

struct rpi_bcm2711_fb_descriptor {
	struct rpi_bcm2711_fb_size display;
	uint32_t depth;
	uint32_t pixel_order;
	uint32_t pitch;
	struct rpi_bcm2711_fb_alloc alloc;
};

struct rpi_bcm2711_fb_config {
	struct display_framebuffer_common_config common;
	enum display_pixel_format pixel_format;
	bool red_blue_swap;
};

struct rpi_bcm2711_fb_data {
	struct display_framebuffer_common_data common;
};

static void rpi_bcm2711_fb_get_capabilities(const struct device *dev,
					    struct display_capabilities *capabilities)
{
	const struct rpi_bcm2711_fb_config *cfg = dev->config;

	capabilities->x_resolution = cfg->common.width;
	capabilities->y_resolution = cfg->common.height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888 | PIXEL_FORMAT_ABGR_8888;
	capabilities->current_pixel_format = cfg->pixel_format;
	capabilities->screen_info = 0;
}

static int rpi_bcm2711_fb_set_pixel_format(const struct device *dev,
					   const enum display_pixel_format pixel_format)
{
	const struct rpi_bcm2711_fb_config *cfg = dev->config;

	if (cfg->pixel_format == pixel_format) {
		return 0;
	}

	LOG_WRN("Changing pixel format is not supported");
	return -ENOTSUP;
}

static int rpi_bcm2711_fb_configure(const struct device *fw_dev,
				    struct rpi_bcm2711_fb_descriptor *fb)
{
	struct rpi_fw_tag_list tag_list[RPI_BCM2711_FB_TAG_SIZE];

	tag_list[0].tag = RPI_FW_TAG_FB_SET_PHYSICAL_SIZE;
	tag_list[0].size = sizeof(uint64_t);
	tag_list[0].data = &fb->display;

	tag_list[1].tag = RPI_FW_TAG_FB_SET_VIRTUAL_SIZE;
	tag_list[1].size = sizeof(uint64_t);
	tag_list[1].data = &fb->display;

	tag_list[2].tag = RPI_FW_TAG_FB_SET_DEPTH;
	tag_list[2].size = sizeof(uint32_t);
	tag_list[2].data = &fb->depth;

	tag_list[3].tag = RPI_FW_TAG_FB_SET_PIXEL_ORDER;
	tag_list[3].size = sizeof(uint32_t);
	tag_list[3].data = &fb->pixel_order;

	tag_list[4].tag = RPI_FW_TAG_FB_ALLOCATE_BUFFER;
	tag_list[4].size = sizeof(uint64_t);
	tag_list[4].data = &fb->alloc;

	tag_list[5].tag = RPI_FW_TAG_FB_GET_PITCH;
	tag_list[5].size = sizeof(uint32_t);
	tag_list[5].data = &fb->pitch;

	return rpi_fw_transfer_list(fw_dev, tag_list, ARRAY_SIZE(tag_list));
}

static int rpi_bcm2711_fb_init(const struct device *dev)
{
	const struct rpi_bcm2711_fb_config *cfg = dev->config;
	struct rpi_bcm2711_fb_data *data = dev->data;
	const struct device *fw_dev = cfg->common.fw_dev;
	struct rpi_bcm2711_fb_descriptor fb;
	int ret;

	fb.display.width = cfg->common.width;
	fb.display.height = cfg->common.height;
	fb.depth = DISPLAY_BITS_PER_PIXEL(cfg->pixel_format);
	fb.pixel_order = cfg->red_blue_swap ? 0 : RPI_BCM2711_FB_PIXEL_ORDER_RGB;
	fb.alloc.buffer = 16;
	fb.alloc.size = 0;
	fb.pitch = 0;

	if (!device_is_ready(fw_dev)) {
		LOG_ERR_DEVICE_NOT_READY(fw_dev);
		return -ENODEV;
	}

	ret = rpi_bcm2711_fb_configure(fw_dev, &fb);
	if (ret < 0) {
		LOG_ERR("Failed to configure framebuffer controller (%d)", ret);
		return ret;
	}

	if (cfg->common.width != fb.display.width || cfg->common.height != fb.display.height) {
		LOG_ERR("Firmware framebuffer size does not match dt-property");
		return -EINVAL;
	}

	if (fb.alloc.buffer == 0 || fb.alloc.size == 0) {
		LOG_ERR("Firmware did not allocate framebuffer");
		return -ENOMEM;
	}

	if (fb.pitch == 0) {
		LOG_ERR("Firmware did not provide framebuffer pitch");
		return -EINVAL;
	}

	LOG_DBG("Framebuffer initialized: width=%u, height=%u, depth=%u, pixel_order=%s, "
		"alloc_buffer=0x%08x, alloc_size=%uKiB, pitch=%u",
		fb.display.width, fb.display.height, fb.depth,
		(fb.pixel_order == RPI_BCM2711_FB_PIXEL_ORDER_RGB) ? "RGB" : "BGR", fb.alloc.buffer,
		fb.alloc.size / 8192, fb.pitch);

	data->common.pitch = (uint16_t)fb.pitch / RPI_BCM2711_FB_BYTES_PER_PIXEL;
	device_map(&data->common.fb_addr, BCM2711_PHYS_ADDR_MASK(fb.alloc.buffer), fb.alloc.size,
		   K_MEM_CACHE_NONE | K_MEM_PERM_RW);

	return 0;
}

static DEVICE_API(display, rpi_bcm2711_fb_api) = {
	.read = display_framebuffer_read,
	.write = display_framebuffer_write,
	.get_capabilities = rpi_bcm2711_fb_get_capabilities,
	.set_pixel_format = rpi_bcm2711_fb_set_pixel_format,
};

#define RPI_BCM2711_FB_DEFINE(n)                                                                   \
	static const struct rpi_bcm2711_fb_config rpi_bcm2711_fb_config_##n = {                    \
		.common = DISPLAY_FRAMEBUFFER_COMMON_CONFIG_FROM_DT_INST(n),                       \
		.pixel_format = DT_INST_PROP(n, pixel_format),                                     \
		.red_blue_swap = DT_INST_PROP(n, red_blue_swap),                                   \
	};                                                                                         \
                                                                                                   \
	static struct rpi_bcm2711_fb_data rpi_bcm2711_fb_data_##n = {                              \
		.common = DISPLAY_FRAMEBUFFER_BYTES_PER_PIXEL(RPI_BCM2711_FB_BYTES_PER_PIXEL),     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &rpi_bcm2711_fb_init, NULL, &rpi_bcm2711_fb_data_##n,             \
			      &rpi_bcm2711_fb_config_##n, POST_KERNEL,                             \
			      CONFIG_DISPLAY_INIT_PRIORITY, &rpi_bcm2711_fb_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_BCM2711_FB_DEFINE)
