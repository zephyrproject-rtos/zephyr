/*
 * Copyright (c) 2025
 *
 * Display driver for EFI GOP (Graphics Output Protocol) framebuffer.
 * Uses GOP info saved by the Zephyr EFI loader (zefi) and maps the
 * framebuffer for use by the display API.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_efi_gop_display

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/arch/x86/efi.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/device.h>

/* Gray (R=128, G=128, B=128) for 32-bit BGR/RGB; filled on driver init */
#define GOP_GRAY_PIXEL	0x00808080U

struct efi_gop_data {
	uint32_t *buffer;  /* mapped framebuffer (virtual) */
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
};

static int efi_gop_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format format)
{
	switch (format) {
	case PIXEL_FORMAT_ARGB_8888:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int efi_gop_set_orientation(const struct device *dev,
				  const enum display_orientation orientation)
{
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void efi_gop_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	struct efi_gop_data *data = dev->data;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = data->width;
	caps->y_resolution = data->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_ARGB_8888;
	caps->screen_info = 0;
	caps->current_pixel_format = PIXEL_FORMAT_ARGB_8888;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int efi_gop_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct efi_gop_data *data = dev->data;
	uint32_t *dst = data->buffer;
	const uint32_t *src = buf;
	uint32_t row;

	dst += x;
	dst += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		(void)memcpy(dst, src, desc->width * sizeof(uint32_t));
		dst += data->pitch;
		src += desc->pitch;
	}

	return 0;
}

static int efi_gop_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	struct efi_gop_data *data = dev->data;
	uint32_t *src = data->buffer;
	uint32_t *dst = buf;
	uint32_t row;

	src += x;
	src += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		(void)memcpy(dst, src, desc->width * sizeof(uint32_t));
		src += data->pitch;
		dst += desc->pitch;
	}

	return 0;
}

static int efi_gop_blanking_on(const struct device *dev)
{
	return 0;
}

static int efi_gop_blanking_off(const struct device *dev)
{
	return 0;
}

static int efi_gop_set_brightness(const struct device *dev, const uint8_t brightness)
{
	return -ENOTSUP;
}

static int efi_gop_set_contrast(const struct device *dev, const uint8_t contrast)
{
	return -ENOTSUP;
}

static DEVICE_API(display, efi_gop_display_api) = {
	.blanking_on = efi_gop_blanking_on,
	.blanking_off = efi_gop_blanking_off,
	.write = efi_gop_write,
	.read = efi_gop_read,
	.get_capabilities = efi_gop_get_capabilities,
	.set_pixel_format = efi_gop_set_pixel_format,
	.set_orientation = efi_gop_set_orientation,
	.set_brightness = efi_gop_set_brightness,
	.set_contrast = efi_gop_set_contrast,
};

static int efi_gop_init(const struct device *dev)
{
	struct efi_gop_data *data = dev->data;
	uint64_t fb_base;
	uintptr_t fb_size;
	uint32_t width, height, pitch;
	uint8_t *mapped = NULL;

	fb_base = efi_get_gop_fb_base();
	fb_size = efi_get_gop_fb_size();
	if (fb_base == 0ULL || fb_size == 0UL) {
		return -ENODEV;
	}

	if (!efi_get_gop_mode(&width, &height, &pitch)) {
		return -ENODEV;
	}

	data->width = width;
	data->height = height;
	data->pitch = pitch;

#ifdef CONFIG_MMU
	k_mem_map_phys_bare(&mapped, (uintptr_t)fb_base, fb_size,
			    K_MEM_PERM_RW);
	if (mapped == NULL) {
		return -ENOMEM;
	}
	data->buffer = (uint32_t *)mapped;
#else
	/* No MMU: assume framebuffer is identity-mapped (e.g. x86 EFI) */
	data->buffer = (uint32_t *)(uintptr_t)fb_base;
#endif

	/* Fill screen with gray on first init (display becomes effective when driver is used) */
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			data->buffer[y * pitch + x] = GOP_GRAY_PIXEL;
		}
	}

	return 0;
}

#define EFI_GOP_DEFINE(n)							\
	static struct efi_gop_data efi_gop_data_##n;				\
	DEVICE_DT_INST_DEFINE(n, efi_gop_init, NULL, &efi_gop_data_##n, NULL,	\
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,	\
			      &efi_gop_display_api);

DT_INST_FOREACH_STATUS_OKAY(EFI_GOP_DEFINE)
