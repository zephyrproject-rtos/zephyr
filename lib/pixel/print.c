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
#include <zephyr/pixel/formats.h>
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

static inline uint8_t pixel_rgb24_to_256color(const uint8_t rgb24[3])
{
	return 16 + rgb24[0] * 6 / 256 * 36 + rgb24[1] * 6 / 256 * 6 + rgb24[2] * 6 / 256 * 1;
}

static inline uint8_t pixel_gray8_to_256color(uint8_t gray8)
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

static inline void pixel_print(const uint8_t *src, size_t size, uint16_t width, uint16_t height,
			       fn_print_t *fn_print, fn_conv_t *fn_conv, int bytespp, int npix)
{
	size_t pitch = width * bytespp;

	for (size_t i = 0, h = 0; h + 2 <= height; h += 2) {
		for (size_t w = 0; w + npix <= width; w += npix, i += bytespp * npix) {
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

static void pixel_rgb24line_to_rgb24line(const uint8_t *rgb24i, uint8_t *rgb24o, uint16_t width)
{
	memcpy(rgb24o, rgb24i, width * 3);
}

void pixel_print_rgb24frame_256color(const uint8_t *rgb24, size_t size, uint16_t width,
				     uint16_t height)
{
	pixel_print(rgb24, size, width, height, pixel_print_256color,
		    pixel_rgb24line_to_rgb24line, 3, 1);
}

void pixel_print_rgb24frame_truecolor(const uint8_t *rgb24, size_t size, uint16_t width,
				      uint16_t height)
{
	pixel_print(rgb24, size, width, height, pixel_print_truecolor,
		    pixel_rgb24line_to_rgb24line, 3, 1);
}

void pixel_print_rgb565leframe_256color(const uint8_t *rgb565, size_t size, uint16_t width,
					uint16_t height)
{
	pixel_print(rgb565, size, width, height, pixel_print_256color,
		    pixel_rgb565leline_to_rgb24line, 2, 1);
}

void pixel_print_rgb565leframe_truecolor(const uint8_t *rgb565, size_t size, uint16_t width,
					 uint16_t height)
{
	pixel_print(rgb565, size, width, height, pixel_print_truecolor,
		    pixel_rgb565leline_to_rgb24line, 2, 1);
}

void pixel_print_rgb565beframe_256color(const uint8_t *rgb565, size_t size, uint16_t width,
					uint16_t height)
{
	pixel_print(rgb565, size, width, height, pixel_print_256color,
		    pixel_rgb565beline_to_rgb24line, 2, 1);
}

void pixel_print_rgb565beframe_truecolor(const uint8_t *rgb565, size_t size, uint16_t width,
					 uint16_t height)
{
	pixel_print(rgb565, size, width, height, pixel_print_truecolor,
		    pixel_rgb565beline_to_rgb24line, 2, 1);
}

void pixel_print_rgb332frame_256color(const uint8_t *rgb332, size_t size, uint16_t width,
				      uint16_t height)
{
	pixel_print(rgb332, size, width, height, pixel_print_256color,
		    pixel_rgb332line_to_rgb24line, 1, 1);
}

void pixel_print_rgb332frame_truecolor(const uint8_t *rgb332, size_t size, uint16_t width,
				       uint16_t height)
{
	pixel_print(rgb332, size, width, height, pixel_print_truecolor,
		    pixel_rgb332line_to_rgb24line, 1, 1);
}

void pixel_print_yuyvframe_bt709_256color(const uint8_t *yuyv, size_t size, uint16_t width,
					  uint16_t height)
{
	pixel_print(yuyv, size, width, height, pixel_print_256color,
		    pixel_yuyvline_to_rgb24line_bt709, 2, 2);
}

void pixel_print_yuyvframe_bt709_truecolor(const uint8_t *yuyv, size_t size, uint16_t width,
					   uint16_t height)
{
	pixel_print(yuyv, size, width, height, pixel_print_truecolor,
		    pixel_yuyvline_to_rgb24line_bt709, 2, 2);
}

void pixel_print_yuv24frame_bt709_256color(const uint8_t *yuv24, size_t size, uint16_t width,
					   uint16_t height)
{
	pixel_print(yuv24, size, width, height, pixel_print_256color,
		    pixel_yuv24line_to_rgb24line_bt709, 3, 1);
}

void pixel_print_yuv24frame_bt709_truecolor(const uint8_t *yuv24, size_t size, uint16_t width,
					    uint16_t height)
{
	pixel_print(yuv24, size, width, height, pixel_print_truecolor,
		    pixel_yuv24line_to_rgb24line_bt709, 3, 1);
}

void pixel_print_raw8frame_hex(const uint8_t *raw8, size_t size, uint16_t width, uint16_t height)
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

void pixel_print_rgb24frame_hex(const uint8_t *rgb24, size_t size, uint16_t width, uint16_t height)
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

void pixel_print_rgb565frame_hex(const uint8_t *rgb565, size_t size, uint16_t width,
				 uint16_t height)
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

void pixel_print_yuyvframe_hex(const uint8_t *yuyv, size_t size, uint16_t width, uint16_t height)
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
