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

static void pixel_print_truecolor(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3])
{
	printf("\e[48;2;%u;%u;%um\e[38;2;%u;%u;%um▄", rgb24row0[0], rgb24row0[1], rgb24row0[2],
	       rgb24row1[0], rgb24row1[1], rgb24row1[2]);
}

static void pixel_print_256color(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3])
{
	printf("\e[48;5;%um\e[38;5;%um▄", pixel_rgb24_to_256color(rgb24row0),
	       pixel_rgb24_to_256color(rgb24row1));
}

static void pixel_print_256gray(uint8_t gray8row0, uint8_t gray8row1)
{
	printf("\e[48;5;%um\e[38;5;%um▄", pixel_gray8_to_256color(gray8row0),
	       pixel_gray8_to_256color(gray8row1));
}

typedef void fn_rgb24_t(const uint8_t rgb24row0[3], const uint8_t rgb24row1[3]);

static inline void pixel_print_rgb24(const uint8_t *rgb24, size_t size, uint16_t width,
				     uint16_t height, fn_rgb24_t *fn)
{
	size_t pitch = width * 3;

	for (size_t i = 0; height - 2 >= 0; height -= 2) {
		for (size_t n = width; n > 0; n--, i += 3) {
			if (i + pitch > size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}
			fn(&rgb24[i], &rgb24[i + pitch]);
		}
		printf("\e[m│\n");

		/* Skip the second h being printed at the same time */
		i += pitch;
	}
}

void pixel_print_rgb24frame_256color(const uint8_t *rgb24, size_t size, uint16_t width,
				     uint16_t height)
{
	pixel_print_rgb24(rgb24, size, width, height, pixel_print_256color);
}

void pixel_print_rgb24frame_truecolor(const uint8_t *rgb24, size_t size, uint16_t width,
				      uint16_t height)
{
	pixel_print_rgb24(rgb24, size, width, height, pixel_print_truecolor);
}

static inline void pixel_print_rgb565(const uint8_t *rgb565, size_t size, uint16_t width,
				      uint16_t height, fn_rgb24_t *fn, bool big_endian)
{
	size_t pitch = width * 2;

	for (size_t i = 0; height - 2 >= 0; height -= 2) {
		for (size_t n = width; n > 0; n--, i += 2) {
			uint8_t rgb24[2][3];

			if (i + pitch > size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			if (big_endian) {
				pixel_rgb565be_to_rgb24(&rgb565[i + pitch * 0], rgb24[0]);
				pixel_rgb565be_to_rgb24(&rgb565[i + pitch * 1], rgb24[1]);
			} else {
				pixel_rgb565le_to_rgb24(&rgb565[i + pitch * 0], rgb24[0]);
				pixel_rgb565le_to_rgb24(&rgb565[i + pitch * 1], rgb24[1]);
			}

			fn(rgb24[0], rgb24[1]);
		}
		printf("\e[m│\n");

		/* Skip the second h being printed at the same time */
		i += pitch;
	}
}

void pixel_print_rgb565leframe_256color(const uint8_t *rgb565, size_t size, uint16_t width,
					uint16_t height)
{
	return pixel_print_rgb565(rgb565, size, width, height, pixel_print_256color, false);
}

void pixel_print_rgb565leframe_truecolor(const uint8_t *rgb565, size_t size, uint16_t width,
					 uint16_t height)
{
	return pixel_print_rgb565(rgb565, size, width, height, pixel_print_truecolor, false);
}

void pixel_print_rgb565beframe_256color(const uint8_t *rgb565, size_t size, uint16_t width,
					uint16_t height)
{
	return pixel_print_rgb565(rgb565, size, width, height, pixel_print_256color, true);
}

void pixel_print_rgb565beframe_truecolor(const uint8_t *rgb565, size_t size, uint16_t width,
					 uint16_t height)
{
	return pixel_print_rgb565(rgb565, size, width, height, pixel_print_truecolor, true);
}

static inline void pixel_print_yuyv(const uint8_t *yuyv, size_t size, uint16_t width,
				    uint16_t height, fn_rgb24_t *fn, int bt)
{
	size_t pitch = width * 2;

	for (size_t i = 0; height - 2 >= 0; height -= 2) {
		for (size_t n = width; n >= 2; n -= 2, i += 4) {
			uint8_t rgb24x2[2][6];

			if (i + pitch > size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			switch (bt) {
			case 601:
				pixel_yuyv_to_rgb24x2_bt601(&yuyv[i + pitch * 0], rgb24x2[0]);
				pixel_yuyv_to_rgb24x2_bt601(&yuyv[i + pitch * 1], rgb24x2[1]);
				break;
			case 709:
				pixel_yuyv_to_rgb24x2_bt709(&yuyv[i + pitch * 0], rgb24x2[0]);
				pixel_yuyv_to_rgb24x2_bt709(&yuyv[i + pitch * 1], rgb24x2[1]);
				break;
			default:
				CODE_UNREACHABLE;
			}

			fn(&rgb24x2[0][0], &rgb24x2[1][0]);
			fn(&rgb24x2[0][3], &rgb24x2[1][3]);
		}
		printf("\e[m│\n");

		/* Skip the second h being printed at the same time */
		i += pitch;
	}
}

void pixel_print_yuyvframe_bt601_256color(const uint8_t *yuyv, size_t size, uint16_t width,
					  uint16_t height)
{
	return pixel_print_yuyv(yuyv, size, width, height, pixel_print_256color, 601);
}

void pixel_print_yuyvframe_bt601_truecolor(const uint8_t *yuyv, size_t size, uint16_t width,
					   uint16_t height)
{
	return pixel_print_yuyv(yuyv, size, width, height, pixel_print_truecolor, 601);
}

void pixel_print_yuyvframe_bt709_256color(const uint8_t *yuyv, size_t size, uint16_t width,
					  uint16_t height)
{
	return pixel_print_yuyv(yuyv, size, width, height, pixel_print_truecolor, 709);
}

void pixel_print_yuyvframe_bt709_truecolor(const uint8_t *yuyv, size_t size, uint16_t width,
					   uint16_t height)
{
	return pixel_print_yuyv(yuyv, size, width, height, pixel_print_truecolor, 709);
}

void pixel_print_raw8frame_hex(const uint8_t *raw8, size_t size, uint16_t width, uint16_t height)
{
	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 1 + w * 1;

			if (i >= size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			printf(" %02x", raw8[i]);
		}
		printf(" row%u\n", h);
	}
}

void pixel_print_rgb24frame_hex(const uint8_t *rgb24, size_t size, uint16_t width, uint16_t height)
{
	printf(" ");
	for (uint16_t w = 0; w < width; w++) {
		printf("col%-7u", w);
	}
	printf("\n");

	for (uint16_t w = 0; w < width; w++) {
		printf(" R  G  B  ");
	}
	printf("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 3 + w * 3;

			if (i + 2 >= size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			printf(" %02x %02x %02x ", rgb24[i + 0], rgb24[i + 1], rgb24[i + 2]);
		}
		printf(" row%u\n", h);
	}
}

void pixel_print_rgb565frame_hex(const uint8_t *rgb565, size_t size, uint16_t width,
				 uint16_t height)
{
	printf(" ");
	for (uint16_t w = 0; w < width; w++) {
		printf("col%-4u", w);
	}
	printf("\n");

	for (uint16_t w = 0; w < width; w++) {
		printf(" RGB565");
	}
	printf("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 2 + w * 2;

			if (i + 1 >= size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			printf(" %02x %02x ", rgb565[i + 0], rgb565[i + 1]);
		}
		printf(" row%u\n", h);
	}
}

void pixel_print_yuyvframe_hex(const uint8_t *yuyv, size_t size, uint16_t width, uint16_t height)
{
	printf(" ");
	for (uint16_t w = 0; w < width; w++) {
		printf("col%-3u", w);
		if ((w + 1) % 2 == 0) {
			printf(" ");
		}
	}
	printf("\n");

	for (uint16_t w = 0; w < width; w++) {
		printf(" %c%u", "YUYV"[w % 2 * 2 + 0], w % 2);
		printf(" %c%u", "YUYV"[w % 2 * 2 + 1], w % 2);
		if ((w + 1) % 2 == 0) {
			printf(" ");
		}
	}
	printf("\n");

	for (uint16_t h = 0; h < height; h++) {
		for (uint16_t w = 0; w < width; w++) {
			size_t i = h * width * 2 + w * 2;

			if (i + 1 >= size) {
				printf("\e[m *** early end of buffer at %zu bytes ***\n", size);
				return;
			}

			printf(" %02x %02x", yuyv[i], yuyv[i + 1]);
			if ((w + 1) % 2 == 0) {
				printf(" ");
			}
		}
		printf(" row%u\n", h);
	}
}

static void pixel_print_hist_scale(size_t size)
{
	for (uint16_t i = 0; i < size; i++) {
		pixel_print_256gray(0, i * 256 / size);
	}
	printf("\e[m\n");
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
		printf("\e[m│ - %u\n", h * max / height);
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
		printf("\e[m│ - %u\n", h * max / height);
	}

	pixel_print_hist_scale(size);
}
