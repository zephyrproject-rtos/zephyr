/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qemu_ramfb

#include <stdint.h>
#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/firmware/qemu_fwcfg/qemu_fwcfg.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

/* QEMU ramfb uses DRM fourcc; AR24 matches Zephyr ARGB_8888 API format. */
#define FOURCC_AR24 0x34325241u /* 'A' 'R' '2' '4' as 0x34 0x32 0x52 0x41 */
#define BPP         4

struct ramfb_config {
	const struct device *fwcfg;
	uint16_t width;
	uint16_t height;
	uintptr_t fb_phys;
	size_t fb_size;
};

struct ramfb_data {
	uint32_t *fb;
	mm_reg_t fb_map;
	uint32_t pitch;
};

struct ramfb_settings {
	uint64_t addr;
	uint32_t fourcc;
	uint32_t flags;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
} __packed;

static int ramfb_set_pixel_format(const struct device *dev, const enum display_pixel_format format)
{
	switch (format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ramfb_set_orientation(const struct device *dev,
				 const enum display_orientation orientation)
{
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void ramfb_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ramfb_config *config = dev->config;

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int ramfb_write(const struct device *dev, uint16_t x, uint16_t y,
		       const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ramfb_config *cfg = dev->config;
	struct ramfb_data *data = dev->data;
	uint32_t *dst = data->fb;
	const uint32_t *src = (const uint32_t *)buf;

	if ((x + desc->width > cfg->width) || (y + desc->height > cfg->height) ||
	    desc->pitch < desc->width) {
		return -EINVAL;
	}
	if (desc->buf_size < ((size_t)desc->pitch * desc->height * BPP)) {
		return -EINVAL;
	}

	dst += x + (y * data->pitch);

	for (uint32_t row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * sizeof(uint32_t));
		dst += data->pitch;
		src += desc->pitch;
	}

	return 0;
}

static int ramfb_read(const struct device *dev, uint16_t x, uint16_t y,
		      const struct display_buffer_descriptor *desc, void *buf)
{
	const struct ramfb_config *cfg = dev->config;
	struct ramfb_data *data = dev->data;
	uint32_t *src = data->fb;
	uint32_t *dst = (uint32_t *)buf;

	if ((x + desc->width > cfg->width) || (y + desc->height > cfg->height) ||
	    desc->pitch < desc->width) {
		return -EINVAL;
	}
	if (desc->buf_size < ((size_t)desc->pitch * desc->height * BPP)) {
		return -EINVAL;
	}

	src += x + (y * data->pitch);

	for (uint32_t row = 0; row < desc->height; row++) {
		memcpy(dst, src, desc->width * sizeof(uint32_t));
		src += data->pitch;
		dst += desc->pitch;
	}

	return 0;
}

static DEVICE_API(display, ramfb_api) = {
	.write = ramfb_write,
	.read = ramfb_read,
	.get_capabilities = ramfb_get_capabilities,
	.set_pixel_format = ramfb_set_pixel_format,
	.set_orientation = ramfb_set_orientation,
};

static int ramfb_init(const struct device *dev)
{
	const struct ramfb_config *cfg = dev->config;
	struct ramfb_data *data = dev->data;
	struct ramfb_settings settings;
	uint16_t select;
	uint32_t size;
	uint8_t *fb_va;

	size_t req_size = (size_t)cfg->width * cfg->height * BPP;

	if (!device_is_ready(cfg->fwcfg)) {
		return -ENODEV;
	}

	if (req_size > cfg->fb_size) {
		return -EINVAL;
	}

	int rc = qemu_fwcfg_find_file(cfg->fwcfg, "etc/ramfb", &select, &size);

	if (rc != 0) {
		return rc;
	}
	if (size != sizeof(struct ramfb_settings)) {
		return -EINVAL;
	}

	device_map(&data->fb_map, cfg->fb_phys, req_size, K_MEM_CACHE_NONE);
	fb_va = (uint8_t *)data->fb_map;
	data->fb = (uint32_t *)fb_va;
	data->pitch = cfg->width;

	settings.addr = sys_cpu_to_be64((uint64_t)cfg->fb_phys);
	settings.fourcc = sys_cpu_to_be32(FOURCC_AR24);
	settings.flags = sys_cpu_to_be32(0);
	settings.width = sys_cpu_to_be32(cfg->width);
	settings.height = sys_cpu_to_be32(cfg->height);
	settings.stride = sys_cpu_to_be32(cfg->width * BPP);

	rc = qemu_fwcfg_write_item(cfg->fwcfg, select, &settings, sizeof(settings));
	if (rc != 0) {
		return rc;
	}

	return 0;
}

#define RAMFB_INIT(inst)                                                                           \
	static struct ramfb_data ramfb_data_##inst;                                                \
                                                                                                   \
	static const struct ramfb_config ramfb_cfg_##inst = {                                      \
		.fwcfg = DEVICE_DT_GET(DT_INST_PHANDLE(inst, fwcfg)),                              \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
		.fb_phys = (uintptr_t)DT_REG_ADDR(DT_INST_PHANDLE(inst, memory_region)),           \
		.fb_size = DT_REG_SIZE(DT_INST_PHANDLE(inst, memory_region))};                     \
	DEVICE_DT_INST_DEFINE(inst, ramfb_init, NULL, &ramfb_data_##inst, &ramfb_cfg_##inst,       \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ramfb_api);

DT_INST_FOREACH_STATUS_OKAY(RAMFB_INIT)
