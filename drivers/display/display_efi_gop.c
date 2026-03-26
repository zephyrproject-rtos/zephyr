/*
 * Copyright (c) 2025 rede97
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Display driver for EFI GOP (Graphics Output Protocol) framebuffer.
 * Uses GOP info saved by the Zephyr EFI loader (zefi) and maps the
 * framebuffer for use by the display API.
 */

#define DT_DRV_COMPAT zephyr_efi_gop_display

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/arch/x86/efi.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/device.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/math_extras.h>

/** Gray fill pixel (R=128, G=128, B=128) for 32-bit BGR/RGB framebuffer. */
#define GOP_GRAY_PIXEL	0x00808080U

/** Driver instance data for the EFI GOP display. */
struct efi_gop_data {
	uint32_t *buffer; /**< Virtual address of the mapped GOP framebuffer. */
	uint32_t pitch;   /**< Pixels per scan line. */
	uint32_t width;   /**< Horizontal resolution in pixels. */
	uint32_t height;  /**< Vertical resolution in pixels. */
	enum display_pixel_format native_pixel_format; /**< Native GOP pixel format. */
};

static inline uint32_t efi_gop_argb_to_abgr(uint32_t pixel)
{
	return (pixel & 0xFF00FF00U) | ((pixel & 0x00FF0000U) >> 16) |
	       ((pixel & 0x000000FFU) << 16);
}

static inline uint32_t efi_gop_native_from_argb(const struct efi_gop_data *data, uint32_t pixel)
{
	if (data->native_pixel_format == PIXEL_FORMAT_ABGR_8888) {
		return efi_gop_argb_to_abgr(pixel);
	}

	return pixel;
}

static inline uint32_t efi_gop_argb_from_native(const struct efi_gop_data *data, uint32_t pixel)
{
	if (data->native_pixel_format == PIXEL_FORMAT_ABGR_8888) {
		return efi_gop_argb_to_abgr(pixel);
	}

	return pixel;
}

static bool efi_gop_buffer_size_ok(const struct display_buffer_descriptor *desc)
{
	size_t row_bytes;
	size_t required = 0U;
	size_t width_bytes;

	if (desc->height == 0U) {
		return true;
	}

	if (size_mul_overflow((size_t)desc->width, sizeof(uint32_t), &width_bytes) ||
	    size_mul_overflow((size_t)desc->pitch, sizeof(uint32_t), &row_bytes) ||
	    size_mul_overflow((size_t)(desc->height - 1U), row_bytes, &required) ||
	    size_add_overflow(required, width_bytes, &required)) {
		return false;
	}

	return required <= desc->buf_size;
}

static int efi_gop_validate_rect(const struct efi_gop_data *data, const uint16_t x,
				 const uint16_t y,
				 const struct display_buffer_descriptor *desc,
				 const void *buf)
{
	uint32_t x_end;
	uint32_t y_end;

	__ASSERT(desc != NULL, "Display descriptor must not be NULL");
	__ASSERT(buf != NULL, "Display buffer must not be NULL");

	if (desc == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (desc->width == 0U || desc->height == 0U) {
		return 0;
	}

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT(desc->width <= data->width, "Width exceeds framebuffer width");
	__ASSERT(desc->height <= data->height, "Height exceeds framebuffer height");

	if (desc->width > desc->pitch || !efi_gop_buffer_size_ok(desc)) {
		return -EINVAL;
	}

	if (u32_add_overflow((uint32_t)x, desc->width, &x_end) ||
	    u32_add_overflow((uint32_t)y, desc->height, &y_end) ||
	    x_end > data->width || y_end > data->height) {
		return -EINVAL;
	}

	return 0;
}

static bool efi_gop_framebuffer_size_ok(uint32_t width, uint32_t height, uint32_t pitch,
					uintptr_t fb_size)
{
	size_t row_bytes;
	size_t required_bytes;

	if (width == 0U || height == 0U || pitch == 0U || pitch < width) {
		return false;
	}

	if (size_mul_overflow((size_t)pitch, sizeof(uint32_t), &row_bytes) ||
	    size_mul_overflow((size_t)height, row_bytes, &required_bytes)) {
		return false;
	}

	return required_bytes <= fb_size;
}

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
	int ret;

	ret = efi_gop_validate_rect(data, x, y, desc, buf);
	if (ret < 0 || desc->width == 0U || desc->height == 0U) {
		return ret;
	}

	dst += x;
	dst += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		if (data->native_pixel_format == PIXEL_FORMAT_ARGB_8888) {
			(void)memcpy(dst, src, desc->width * sizeof(uint32_t));
		} else {
			for (uint32_t col = 0; col < desc->width; ++col) {
				dst[col] = efi_gop_native_from_argb(data, src[col]);
			}
		}
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
	int ret;

	ret = efi_gop_validate_rect(data, x, y, desc, buf);
	if (ret < 0 || desc->width == 0U || desc->height == 0U) {
		return ret;
	}

	src += x;
	src += (y * data->pitch);

	for (row = 0; row < desc->height; ++row) {
		if (data->native_pixel_format == PIXEL_FORMAT_ARGB_8888) {
			(void)memcpy(dst, src, desc->width * sizeof(uint32_t));
		} else {
			for (uint32_t col = 0; col < desc->width; ++col) {
				dst[col] = efi_gop_argb_from_native(data, src[col]);
			}
		}
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
	enum efi_gop_display_format gop_format;
	uint8_t *mapped = NULL;

	fb_base = efi_get_gop_fb_base();
	fb_size = efi_get_gop_fb_size();
	if (fb_base == 0ULL || fb_size == 0UL) {
		return -ENODEV;
	}

	if (!efi_get_gop_mode(&width, &height, &pitch)) {
		return -ENODEV;
	}

	if (!efi_gop_framebuffer_size_ok(width, height, pitch, fb_size)) {
		return -EINVAL;
	}

	gop_format = efi_get_gop_pixel_format();
	switch (gop_format) {
	case EFI_GOP_DISPLAY_FORMAT_ARGB_8888:
		data->native_pixel_format = PIXEL_FORMAT_ARGB_8888;
		break;
	case EFI_GOP_DISPLAY_FORMAT_ABGR_8888:
		data->native_pixel_format = PIXEL_FORMAT_ABGR_8888;
		break;
	default:
		return -ENODEV;
	}

	data->width = width;
	data->height = height;
	data->pitch = pitch;

#ifdef CONFIG_MMU
	k_mem_map_phys_bare(&mapped, (uintptr_t)fb_base, fb_size,
			    K_MEM_PERM_RW | K_MEM_CACHE_NONE);
	if (mapped == NULL) {
		return -ENOMEM;
	}
	data->buffer = (uint32_t *)mapped;
#else
	/* No MMU: assume framebuffer is identity-mapped (e.g. x86 EFI) */
	data->buffer = (uint32_t *)(uintptr_t)fb_base;
#endif

	/* Fill screen with gray on driver init. */
	for (uint32_t row = 0; row < height; row++) {
		for (uint32_t col = 0; col < width; col++) {
			data->buffer[row * pitch + col] =
				efi_gop_native_from_argb(data, GOP_GRAY_PIXEL);
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
