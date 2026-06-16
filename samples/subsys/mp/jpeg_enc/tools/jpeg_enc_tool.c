/*
 * jpeg_enc_tool - native (Linux) test harness for the libMP jpeg_enc port.
 *
 * Links directly against the in-tree, standalone bitbank-JPEGENC port
 * (subsys/mp/plugins/zjpeg/jpeg_enc.c) and exercises its public C API the same
 * way mp_zjpeg_encoder does. Lets you encode an image to JPEG outside Zephyr.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Meta Platforms, Inc.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/mp/zjpeg/jpeg_enc.h>

/* Pack one RGB888 sample into a native-endian RGB565 word (R in high bits). */
static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
	return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

/* Fill an RGB565 raster with 8 vertical SMPTE-style colour bars. */
static void make_test_pattern(uint16_t *buf, int w, int h)
{
	static const uint8_t bars[8][3] = {
		{255, 255, 255}, {255, 255, 0}, {0, 255, 255}, {0, 255, 0},
		{255, 0, 255},   {255, 0, 0},   {0, 0, 255},   {0, 0, 0},
	};
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			int bar = (x * 8) / (w > 0 ? w : 1);
			if (bar > 7) {
				bar = 7;
			}
			buf[y * w + x] = rgb565(bars[bar][0], bars[bar][1], bars[bar][2]);
		}
	}
}

/* Read a binary PPM (P6, maxval 255) and convert it to an RGB565 raster. */
static uint16_t *read_ppm(const char *path, int *pw, int *ph)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "cannot open %s\n", path);
		return NULL;
	}
	char magic[3] = {0};
	int w = 0, h = 0, maxv = 0;
	if (fscanf(f, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
		fprintf(stderr, "%s: not a P6 PPM\n", path);
		fclose(f);
		return NULL;
	}
	if (fscanf(f, "%d %d %d", &w, &h, &maxv) != 3 || w <= 0 || h <= 0 || maxv != 255) {
		fprintf(stderr, "%s: bad PPM header\n", path);
		fclose(f);
		return NULL;
	}
	fgetc(f); /* consume the single whitespace after maxval */

	uint16_t *out = malloc((size_t)w * h * sizeof(uint16_t));
	uint8_t rgb[3];
	if (!out) {
		fclose(f);
		return NULL;
	}
	for (int i = 0; i < w * h; i++) {
		if (fread(rgb, 1, 3, f) != 3) {
			fprintf(stderr, "%s: short read at pixel %d\n", path, i);
			free(out);
			fclose(f);
			return NULL;
		}
		out[i] = rgb565(rgb[0], rgb[1], rgb[2]);
	}
	fclose(f);
	*pw = w;
	*ph = h;
	return out;
}

static void usage(const char *p)
{
	fprintf(stderr,
		"usage: %s [--in file.ppm] [--width W --height H] [--out out.jpg]\n"
		"          [--quality best|high|med|low] [--subsample 420|444]\n"
		"  no --in: generates an 8-bar colour test pattern (needs --width/--height,\n"
		"           defaults 320x240).\n"
		"  --in file.ppm: encodes a binary P6 PPM (convert any image with e.g.\n"
		"           'ffmpeg -i in.png out.ppm' or ImageMagick 'convert in.png out.ppm').\n",
		p);
}

int main(int argc, char **argv)
{
	const char *in = NULL, *out = "out.jpg";
	int w = 320, h = 240;
	uint8_t q = JPEGE_Q_BEST, ss = JPEGE_SUBSAMPLE_420;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--in") && i + 1 < argc) {
			in = argv[++i];
		} else if (!strcmp(argv[i], "--out") && i + 1 < argc) {
			out = argv[++i];
		} else if (!strcmp(argv[i], "--width") && i + 1 < argc) {
			w = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "--height") && i + 1 < argc) {
			h = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "--quality") && i + 1 < argc) {
			const char *v = argv[++i];
			q = !strcmp(v, "best") ? JPEGE_Q_BEST
			  : !strcmp(v, "high") ? JPEGE_Q_HIGH
			  : !strcmp(v, "med")  ? JPEGE_Q_MED
			  : !strcmp(v, "low")  ? JPEGE_Q_LOW
			  : (uint8_t)0xff;
			if (q == 0xff) {
				usage(argv[0]);
				return 2;
			}
		} else if (!strcmp(argv[i], "--subsample") && i + 1 < argc) {
			const char *v = argv[++i];
			if (!strcmp(v, "420")) {
				ss = JPEGE_SUBSAMPLE_420;
			} else if (!strcmp(v, "444")) {
				ss = JPEGE_SUBSAMPLE_444;
			} else {
				usage(argv[0]);
				return 2;
			}
		} else {
			usage(argv[0]);
			return 2;
		}
	}

	uint16_t *pixels = NULL;
	if (in) {
		pixels = read_ppm(in, &w, &h);
		if (!pixels) {
			return 1;
		}
	} else {
		if (w <= 0 || h <= 0) {
			usage(argv[0]);
			return 2;
		}
		pixels = malloc((size_t)w * h * sizeof(uint16_t));
		if (!pixels) {
			return 1;
		}
		make_test_pattern(pixels, w, h);
	}

	/* Worst-case output budget: raw 24-bit size + slack, never below 1024. */
	int outsize = w * h * 3 + 65536;
	uint8_t *outbuf = malloc(outsize);
	if (!outbuf) {
		free(pixels);
		return 1;
	}

	JPEGE_IMAGE jpe;
	JPEGENCODE enc;
	memset(&jpe, 0, sizeof(jpe));

	if (JPEGOpenRAM(&jpe, outbuf, outsize) != JPEGE_SUCCESS) {
		fprintf(stderr, "JPEGOpenRAM failed\n");
		return 1;
	}
	if (JPEGEncodeBegin(&jpe, &enc, w, h, JPEGE_PIXEL_RGB565, ss, q) != JPEGE_SUCCESS) {
		fprintf(stderr, "JPEGEncodeBegin failed (err %d)\n", JPEGGetLastError(&jpe));
		return 1;
	}
	if (JPEGEncodeFrame(&jpe, &enc, (uint8_t *)pixels, w * 2) != JPEGE_SUCCESS) {
		fprintf(stderr, "JPEGEncodeFrame failed (err %d)\n", JPEGGetLastError(&jpe));
		return 1;
	}
	int jpeg_size = JPEGEncodeEnd(&jpe);
	if (jpeg_size <= 0) {
		fprintf(stderr, "JPEGEncodeEnd failed (err %d)\n", JPEGGetLastError(&jpe));
		return 1;
	}

	FILE *fo = fopen(out, "wb");
	if (!fo || fwrite(outbuf, 1, jpeg_size, fo) != (size_t)jpeg_size) {
		fprintf(stderr, "cannot write %s\n", out);
		return 1;
	}
	fclose(fo);

	printf("encoded %dx%d RGB565 -> %s : %d bytes (quality=%d subsample=%s)\n",
	       w, h, out, jpeg_size, q, ss == JPEGE_SUBSAMPLE_420 ? "420" : "444");

	free(pixels);
	free(outbuf);
	return 0;
}
