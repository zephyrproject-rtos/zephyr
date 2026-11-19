/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#include <zephyr_jpegdec.h>
/* The module's test images use the Arduino PROGMEM storage annotation */
#define PROGMEM
/* tulips[]: the module's 640x480 4:2:0 baseline test image */
#include <tulips.h>

#define TULIPS_WIDTH  640
#define TULIPS_HEIGHT 480

/*
 * CRC-32 of the full-size RGB565 little-endian output. The library's 32-bit
 * and 64-bit builds decode 139 of the 307,200 pixels differently.
 */
#if defined(__x86_64__) || defined(__aarch64__)
#define TULIPS_RGB565_LE_CRC32 0x9b335776U
#else
#define TULIPS_RGB565_LE_CRC32 0xd3a45e5cU
#endif

struct decode_ctx {
	/* Expected output size */
	int32_t width;
	int32_t height;
	/* Per-pixel delivery count */
	uint8_t *coverage;
	/* If not NULL, receives the output in raster order */
	uint16_t *image;
	/* If not NULL, the little-endian output the byte-swapped output must match */
	const uint16_t *reference;
	uint32_t pixels;
	bool out_of_bounds;
	bool repeated;
	bool mismatch;
};

static struct zephyr_jpegdec dec;
static uint16_t tulips_le[TULIPS_WIDTH * TULIPS_HEIGHT];

static int draw(const struct zephyr_jpegdec_block *block, void *user_data)
{
	struct decode_ctx *ctx = user_data;

	if ((block->x < 0) || (block->y < 0) || (block->x + block->width > ctx->width) ||
	    (block->y + block->height > ctx->height)) {
		ctx->out_of_bounds = true;
		return -EINVAL;
	}

	for (int32_t row = 0; row < block->height; row++) {
		const uint16_t *src = &block->pixels[row * block->pitch];
		size_t offset = (size_t)(block->y + row) * (size_t)ctx->width + (size_t)block->x;

		for (int32_t col = 0; col < block->width; col++) {
			if (ctx->coverage[offset + col] != 0U) {
				ctx->repeated = true;
			}
			ctx->coverage[offset + col] = 1U;
			if (ctx->image != NULL) {
				ctx->image[offset + col] = src[col];
			}
			if ((ctx->reference != NULL) &&
			    (src[col] != BSWAP_16(ctx->reference[offset + col]))) {
				ctx->mismatch = true;
			}
		}
	}
	ctx->pixels += (uint32_t)block->width * (uint32_t)block->height;

	return 0;
}

static void decode_tulips(enum zephyr_jpegdec_scale scale, bool big_endian, struct decode_ctx *ctx)
{
	static uint8_t coverage[TULIPS_WIDTH * TULIPS_HEIGHT];
	uint16_t width;
	uint16_t height;
	int ret;

	memset(coverage, 0, sizeof(coverage));
	ctx->width = TULIPS_WIDTH / (int32_t)scale;
	ctx->height = TULIPS_HEIGHT / (int32_t)scale;
	ctx->coverage = coverage;

	ret = zephyr_jpegdec_open(&dec, tulips, sizeof(tulips));
	zassert_equal(ret, 0, "open failed: %d", ret);
	zephyr_jpegdec_get_size(&dec, &width, &height);
	zassert_equal(width, TULIPS_WIDTH);
	zassert_equal(height, TULIPS_HEIGHT);
	ret = zephyr_jpegdec_decode_rgb565(&dec, big_endian, scale, draw, ctx);
	zassert_equal(ret, 0, "decode failed: %d", ret);
	zephyr_jpegdec_close(&dec);

	zassert_false(ctx->out_of_bounds, "block outside the %dx%d output", ctx->width,
		      ctx->height);
	zassert_false(ctx->repeated, "pixel delivered more than once");
	zassert_equal(ctx->pixels, (uint32_t)(ctx->width * ctx->height),
		      "%u pixels delivered for a %dx%d output", ctx->pixels, ctx->width,
		      ctx->height);
}

ZTEST(jpegdec, test_full_size_rgb565)
{
	struct decode_ctx ctx = {.image = tulips_le};
	uint32_t crc = 0U;

	decode_tulips(ZEPHYR_JPEGDEC_SCALE_1, false, &ctx);

	for (size_t i = 0; i < ARRAY_SIZE(tulips_le); i++) {
		uint8_t le[2];

		sys_put_le16(tulips_le[i], le);
		crc = crc32_ieee_update(crc, le, sizeof(le));
	}
	zassert_equal(crc, TULIPS_RGB565_LE_CRC32, "output CRC-32 0x%08x", crc);
}

ZTEST(jpegdec, test_big_endian_rgb565)
{
	struct decode_ctx le_ctx = {.image = tulips_le};
	struct decode_ctx be_ctx = {.reference = tulips_le};

	decode_tulips(ZEPHYR_JPEGDEC_SCALE_1, false, &le_ctx);
	decode_tulips(ZEPHYR_JPEGDEC_SCALE_1, true, &be_ctx);
	zassert_false(be_ctx.mismatch, "big-endian output is not the byte-swapped output");
}

ZTEST(jpegdec, test_scaled)
{
	static const enum zephyr_jpegdec_scale scales[] = {
		ZEPHYR_JPEGDEC_SCALE_1_2,
		ZEPHYR_JPEGDEC_SCALE_1_4,
		ZEPHYR_JPEGDEC_SCALE_1_8,
	};

	for (size_t i = 0; i < ARRAY_SIZE(scales); i++) {
		struct decode_ctx ctx = {0};

		decode_tulips(scales[i], false, &ctx);
	}
}

ZTEST(jpegdec, test_no_components)
{
	static uint8_t data[sizeof(tulips)];
	size_t sof = 2;

	memcpy(data, tulips, sizeof(data));
	while ((sof + 1 < sizeof(data)) && !((data[sof] == 0xffU) && (data[sof + 1] == 0xc0U))) {
		sof++;
	}
	zassert_true(sof + 10 < sizeof(data), "no SOF0 marker");

	/* Marker, length, precision, height and width come before the component count */
	data[sof + 9] = 0U;
	zassert_equal(zephyr_jpegdec_open(&dec, data, sizeof(data)), -EBADMSG,
		      "frame header without components accepted");
}

ZTEST(jpegdec, test_draw_error)
{
	struct decode_ctx ctx = {.width = 1, .height = 1};

	zassert_equal(zephyr_jpegdec_open(&dec, tulips, sizeof(tulips)), 0);
	/* A 1x1 expected output makes the first block out of bounds */
	zassert_equal(zephyr_jpegdec_decode_rgb565(&dec, false, ZEPHYR_JPEGDEC_SCALE_1, draw, &ctx),
		      -EINVAL);
	zephyr_jpegdec_close(&dec);
}

ZTEST(jpegdec, test_yuv420p_nv12)
{
	static uint8_t y420[TULIPS_WIDTH * TULIPS_HEIGHT];
	static uint8_t u420[(TULIPS_WIDTH / 2) * (TULIPS_HEIGHT / 2)];
	static uint8_t v420[(TULIPS_WIDTH / 2) * (TULIPS_HEIGHT / 2)];
	static uint8_t y12[TULIPS_WIDTH * TULIPS_HEIGHT];
	static uint8_t uv12[TULIPS_WIDTH * (TULIPS_HEIGHT / 2)];

	zassert_equal(zephyr_jpegdec_open(&dec, tulips, sizeof(tulips)), 0);
	zassert_equal(zephyr_jpegdec_decode_yuv420p(&dec, y420, TULIPS_WIDTH, u420,
						    TULIPS_WIDTH / 2, v420, TULIPS_WIDTH / 2),
		      0);
	zephyr_jpegdec_close(&dec);

	zassert_equal(zephyr_jpegdec_open(&dec, tulips, sizeof(tulips)), 0);
	zassert_equal(zephyr_jpegdec_decode_nv12(&dec, y12, TULIPS_WIDTH, uv12, TULIPS_WIDTH), 0);
	zephyr_jpegdec_close(&dec);

	zassert_mem_equal(y420, y12, sizeof(y420), "luma planes differ");
	for (size_t i = 0; i < ARRAY_SIZE(u420); i++) {
		zassert_equal(uv12[2 * i], u420[i], "Cb differs at %zu", i);
		zassert_equal(uv12[2 * i + 1], v420[i], "Cr differs at %zu", i);
	}
}

ZTEST_SUITE(jpegdec, NULL, NULL, NULL, NULL, NULL);
