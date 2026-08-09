/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qemu_ramfb

#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "display_framebuffer.h"
#include <zephyr/drivers/firmware/qemu_fwcfg/qemu_fwcfg.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(qemu_ramfb, CONFIG_DISPLAY_LOG_LEVEL);

/* QEMU ramfb uses DRM fourcc; AR24 matches Zephyr ARGB_8888 API format. */
#define QEMU_RAMFB_FOURCC_AR24 0x34325241u /* 'A' 'R' '2' '4' as 0x34 0x32 0x52 0x41 */
#define QEMU_RAMFB_BPP         4

struct ramfb_config {
	struct display_framebuffer_common_config common;
	uintptr_t fb_phys;
	size_t fb_size;
};

struct ramfb_data {
	struct display_framebuffer_common_data common;
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
		LOG_ERR("Pixel format not supported");
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
		LOG_ERR("Pixel orientation not supported");
		return -ENOTSUP;
	}
}

static void ramfb_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ramfb_config *cfg = dev->config;

	caps->x_resolution = cfg->common.width;
	caps->y_resolution = cfg->common.height;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static DEVICE_API(display, ramfb_api) = {
	.read = display_framebuffer_read,
	.write = display_framebuffer_write,
	.get_capabilities = ramfb_get_capabilities,
	.set_pixel_format = ramfb_set_pixel_format,
	.set_orientation = ramfb_set_orientation,
};

static int ramfb_init(const struct device *dev)
{
	const struct ramfb_config *cfg = dev->config;
	struct ramfb_data *data = dev->data;
	const struct device *fwcfg = cfg->common.fw_dev;
	struct ramfb_settings settings;
	uint16_t select;
	uint32_t size;

	size_t req_size = (size_t)cfg->common.width * cfg->common.height * QEMU_RAMFB_BPP;

	if (!device_is_ready(fwcfg)) {
		LOG_ERR_DEVICE_NOT_READY(fwcfg);
		return -ENODEV;
	}

	if (req_size > cfg->fb_size) {
		LOG_ERR("Requested framebuffer size %zu exceeds available size %zu",
			req_size, cfg->fb_size);
		return -EINVAL;
	}

	int rc = qemu_fwcfg_find_file(fwcfg, "etc/ramfb", &select, &size);

	if (rc != 0) {
		LOG_ERR("Failed to lookup ramfb fwcfg (%d)", rc);
		return rc;
	}
	if (size != sizeof(struct ramfb_settings)) {
		LOG_ERR("ramfb size mismatch: %u != %zu", size, sizeof(struct ramfb_settings));
		return -EINVAL;
	}

	settings.addr = sys_cpu_to_be64((uint64_t)cfg->fb_phys);
	settings.fourcc = sys_cpu_to_be32(QEMU_RAMFB_FOURCC_AR24);
	settings.flags = sys_cpu_to_be32(0);
	settings.width = sys_cpu_to_be32(cfg->common.width);
	settings.height = sys_cpu_to_be32(cfg->common.height);
	settings.stride = sys_cpu_to_be32(cfg->common.width * QEMU_RAMFB_BPP);

	rc = qemu_fwcfg_write_item(fwcfg, select, &settings, sizeof(settings));
	if (rc != 0) {
		LOG_ERR("Failed to write ramfb setting (%d)", rc);
		return rc;
	}

	device_map(&data->common.fb_addr, cfg->fb_phys, req_size, K_MEM_CACHE_NONE);
	data->common.pitch = cfg->common.width;

	return 0;
}

#define RAMFB_INIT(inst)                                                                           \
	static struct ramfb_data ramfb_data_##inst = {                                             \
		.common = DISPLAY_FRAMEBUFFER_BYTES_PER_PIXEL(QEMU_RAMFB_BPP),                     \
	};                                                                                         \
                                                                                                   \
	static const struct ramfb_config ramfb_cfg_##inst = {                                      \
		.common = DISPLAY_FRAMEBUFFER_COMMON_CONFIG_FROM_DT_INST(inst),                    \
		.fb_phys = (uintptr_t)DT_REG_ADDR(DT_INST_PHANDLE(inst, memory_region)),           \
		.fb_size = DT_REG_SIZE(DT_INST_PHANDLE(inst, memory_region))};                     \
	DEVICE_DT_INST_DEFINE(inst, ramfb_init, NULL, &ramfb_data_##inst, &ramfb_cfg_##inst,       \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ramfb_api);

DT_INST_FOREACH_STATUS_OKAY(RAMFB_INIT)
