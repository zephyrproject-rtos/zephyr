/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/pixel/print.h>
#include <zephyr/pixel/convert.h>
#include <zephyr/shell/shell.h>

#ifdef CONFIG_PIXEL_PRINT_NONE
#define PIXEL_PRINT(...)
#endif

#ifdef CONFIG_PIXEL_PRINT_PRINTF
#define PIXEL_PRINT(...) printf(__VA_ARGS__)
#endif

#ifdef CONFIG_PIXEL_PRINT_PRINTK
#define PIXEL_PRINT(...) printk(__VA_ARGS__)
#endif

#ifdef CONFIG_PIXEL_PRINT_SHELL
#define PIXEL_PRINT(...) shell_print(pixel_print_shell, __VA_ARGS__)
#endif

static struct shell *pixel_print_shell;

void pixel_print_set_shell(struct shell *sh)
{
	pixel_print_shell = sh;
}

__unused static uint8_t pixel_rgb24_to_256color(const uint8_t rgb24[3])
{
	return 16 + rgb24[0] * 6 / 256 * 36 + rgb24[1] * 6 / 256 * 6 + rgb24[2] * 6 / 256 * 1;
}

__unused static uint8_t pixel_gray8_to_256color(uint8_t gray8)
{
	return 232 + gray8 * 24 / 256;
}

static void pixel_print_truecolor(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3])
{
	PIXEL_PRINT("\e[48;2;%u;%u;%um\e[38;2;%u;%u;%um▄",
		    rgb24row0[0], rgb24row0[1], rgb24row0[2],
		    rgb24row1[0], rgb24row1[1], rgb24row1[2]);
}

static void pixel_print_256color(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3])
{
	PIXEL_PRINT("\e[48;5;%um\e[38;5;%um▄",
		    pixel_rgb24_to_256color(rgb24row0),
		    pixel_rgb24_to_256color(rgb24row1));
}

static void pixel_print_256gray(uint8_t gray8row0, uint8_t gray8row1)
{
	PIXEL_PRINT("\e[48;5;%um\e[38;5;%um▄",
		    pixel_gray8_to_256color(gray8row0),
		    pixel_gray8_to_256color(gray8row1));
}

typedef void fn_print_t(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3]);

typedef void fn_conv_t(const uint8_t *src, uint8_t *dst, uint16_t w);

static void pixel_print(const uint8_t *src, size_t size, uint16_t width, uint16_t height,
			fn_print_t *fn_print, fn_conv_t *fn_conv, int bitspp, int npix)
{
	size_t pitch = width * bitspp / BITS_PER_BYTE;
	uint8_t nbytes = npix * bitspp  / BITS_PER_BYTE;

	for (size_t i = 0, h = 0; h + 2 <= height; h += 2) {
		for (size_t w = 0; w + npix <= width; w += npix, i += nbytes) {
			uint8_t rgb24a[3 * 2], rgb24b[3 * 2];

			__ASSERT_NO_MSG(npix <= 2);

			fn_conv(&src[i + pitch * 0], rgb24a, npix);
			fn_conv(&src[i + pitch * 1], rgb24b, npix);

			if (i + pitch > size) {
				PIXEL_PRINT("\e[m *** early end of buffer at %zu bytes ***\n",
					    size);
				return;
			}

			for (int n = 0; n < npix; n++) {
				fn_print(&rgb24a[n * 3], &rgb24b[n * 3]);
			}
		}
		PIXEL_PRINT("\e[m|\n");

		/* Skip the second h being printed at the same time */
		i += pitch;
	}
}

static void pixel_print_buffer(const uint8_t *buffer, size_t size, uint16_t width, uint16_t height,
			       uint32_t fourcc, fn_print_t *fn)
{
	switch (fourcc) {
	case VIDEO_PIX_FMT_RGB24:
		pixel_print(buffer, size, width, height, fn, pixel_line_rgb24_to_rgb24,
			    video_bits_per_pixel(fourcc), 1);
		break;
	case VIDEO_PIX_FMT_RGB565:
		pixel_print(buffer, size, width, height, fn, pixel_line_rgb565le_to_rgb24,
			    video_bits_per_pixel(fourcc), 1);
		break;
	case VIDEO_PIX_FMT_RGB565X:
		pixel_print(buffer, size, width, height, fn, pixel_line_rgb565be_to_rgb24,
			    video_bits_per_pixel(fourcc), 1);
		break;
	case VIDEO_PIX_FMT_RGB332:
		pixel_print(buffer, size, width, height, fn, pixel_line_rgb332_to_rgb24,
			    video_bits_per_pixel(fourcc), 1);
		break;
	case VIDEO_PIX_FMT_YUYV:
		pixel_print(buffer, size, width, height, fn, pixel_line_yuyv_to_rgb24_bt709,
			    video_bits_per_pixel(fourcc), 2);
		break;
	case VIDEO_PIX_FMT_YUV24:
		pixel_print(buffer, size, width, height, fn, pixel_line_yuv24_to_rgb24_bt709,
			    video_bits_per_pixel(fourcc), 1);
		break;
	case VIDEO_PIX_FMT_RGGB8:
	case VIDEO_PIX_FMT_BGGR8:
	case VIDEO_PIX_FMT_GBRG8:
	case VIDEO_PIX_FMT_GRBG8:
	case VIDEO_PIX_FMT_GREY:
		pixel_print(buffer, size, width, height, fn, pixel_line_y8_to_rgb24_bt709,
			    video_bits_per_pixel(fourcc), 1);
		break;
	default:
		PIXEL_PRINT("Printing %s buffers not supported\n", VIDEO_FOURCC_TO_STR(fourcc));
	}
}

void pixel_print_buffer_truecolor(const uint8_t *buffer, size_t size, uint16_t width,
				  uint16_t height, uint32_t fourcc)
{
	pixel_print_buffer(buffer, size, width, height, fourcc, pixel_print_truecolor);
}

void pixel_print_buffer_256color(const uint8_t *buffer, size_t size, uint16_t width,
				  uint16_t height, uint32_t fourcc)
{
	pixel_print_buffer(buffer, size, width, height, fourcc, pixel_print_256color);
}

void pixel_image_print_truecolor(struct pixel_image *img)
{
	pixel_print_buffer_truecolor(img->buffer, img->size, img->width, img->height, img->fourcc);
}

void pixel_image_print_256color(struct pixel_image *img)
{
	pixel_print_buffer_256color(img->buffer, img->size, img->width, img->height, img->fourcc);
}

void pixel_hexdump_raw8(const uint8_t *raw8, size_t size, uint16_t width, uint16_t height)
{
	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 1 + w * 1;

			if (i >= size) {
				PIXEL_PRINT("\e[m *** early end of buffer at %zu bytes ***\n",
					    size);
				return;
			}

			PIXEL_PRINT(" %02x", raw8[i]);
		}
		PIXEL_PRINT(" row%u\n", h);
	}
}

void pixel_hexdump_rgb24(const uint8_t *rgb24, size_t size, uint16_t width, uint16_t height)
{
	PIXEL_PRINT(" ");
	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT("col%-7u", w);
	}
	PIXEL_PRINT("\n");

	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT(" R  G  B  ");
	}
	PIXEL_PRINT("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 3 + w * 3;

			if (i + 2 >= size) {
				PIXEL_PRINT("\e[m *** early end of buffer at %zu bytes ***\n",
					    size);
				return;
			}

			PIXEL_PRINT(" %02x %02x %02x ", rgb24[i + 0], rgb24[i + 1], rgb24[i + 2]);
		}
		PIXEL_PRINT(" row%u\n", h);
	}
}

void pixel_hexdump_rgb565(const uint8_t *rgb565, size_t size, uint16_t width, uint16_t height)
{
	PIXEL_PRINT(" ");
	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT("col%-4u", w);
	}
	PIXEL_PRINT("\n");

	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT(" RGB565");
	}
	PIXEL_PRINT("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 2 + w * 2;

			if (i + 1 >= size) {
				PIXEL_PRINT("\e[m *** early end of buffer at %zu bytes ***\n",
					    size);
				return;
			}

			PIXEL_PRINT(" %02x %02x ", rgb565[i + 0], rgb565[i + 1]);
		}
		PIXEL_PRINT(" row%u\n", h);
	}
}

void pixel_hexdump_yuyv(const uint8_t *yuyv, size_t size, uint16_t width, uint16_t height)
{
	PIXEL_PRINT(" ");
	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT("col%-3u", w);
		if ((w + 1) % 2 == 0) {
			PIXEL_PRINT(" ");
		}
	}
	PIXEL_PRINT("\n");

	for (uint16_t w = 0; w < width; w++) {
		PIXEL_PRINT(" %c%u", "YUYV"[w % 2 * 2 + 0], w % 2);
		PIXEL_PRINT(" %c%u", "YUYV"[w % 2 * 2 + 1], w % 2);
		if ((w + 1) % 2 == 0) {
			PIXEL_PRINT(" ");
		}
	}
	PIXEL_PRINT("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 2 + w * 2;

			if (i + 1 >= size) {
				PIXEL_PRINT("\e[m *** early end of buffer at %zu bytes ***\n",
					    size);
				return;
			}

			PIXEL_PRINT(" %02x %02x", yuyv[i], yuyv[i + 1]);
			if ((w + 1) % 2 == 0) {
				PIXEL_PRINT(" ");
			}
		}
		PIXEL_PRINT(" row%u\n", h);
	}
}

static void pixel_print_hist_scale(size_t size)
{
	for (uint16_t i = 0; i < size; i++) {
		pixel_print_256gray(0, i * 256 / size);
	}
	PIXEL_PRINT("\e[m\n");
}

void pixel_print_rgb24hist(const uint16_t *rgb24hist, size_t size, uint16_t height)
{
	const uint16_t *r8hist = &rgb24hist[size / 3 * 0];
	const uint16_t *g8hist = &rgb24hist[size / 3 * 1];
	const uint16_t *b8hist = &rgb24hist[size / 3 * 2];
	uint32_t max = 1;

	__ASSERT(size % 3 == 0, "Each of R, G, B channel should have the same size.");

	for (size_t i = 0; i < size; i++) {
		max = rgb24hist[i] > max ? rgb24hist[i] : max;
	}

	for (uint16_t h = height; h > 1; h--) {
		for (size_t i = 0; i < size / 3; i++) {
			uint8_t rgb24row0[3];
			uint8_t rgb24row1[3];

			rgb24row0[0] = (r8hist[i] * height / max > h - 0) ? 0xff : 0x00;
			rgb24row0[1] = (g8hist[i] * height / max > h - 0) ? 0xff : 0x00;
			rgb24row0[2] = (b8hist[i] * height / max > h - 0) ? 0xff : 0x00;
			rgb24row1[0] = (r8hist[i] * height / max > h - 1) ? 0xff : 0x00;
			rgb24row1[1] = (g8hist[i] * height / max > h - 1) ? 0xff : 0x00;
			rgb24row1[2] = (b8hist[i] * height / max > h - 1) ? 0xff : 0x00;

			pixel_print_256color(rgb24row0, rgb24row1);
		}
		PIXEL_PRINT("\e[m| - %u\n", h * max / height);
	}

	pixel_print_hist_scale(size / 3);
}

void pixel_print_y8hist(const uint16_t *y8hist, size_t size, uint16_t height)
{
	uint32_t max = 1;

	for (size_t i = 0; i < size; i++) {
		max = y8hist[i] > max ? y8hist[i] : max;
	}

	for (uint16_t h = height; h > 1; h--) {
		for (size_t i = 0; i < size; i++) {
			uint8_t gray8row0 = (y8hist[i] * height / max > h - 0) ? 0xff : 0x00;
			uint8_t gray8row1 = (y8hist[i] * height / max > h - 1) ? 0xff : 0x00;

			pixel_print_256gray(gray8row0, gray8row1);
		}
		PIXEL_PRINT("\e[m| - %u\n", h * max / height);
	}

	pixel_print_hist_scale(size);
}
