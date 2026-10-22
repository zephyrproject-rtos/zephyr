/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/drivers/display/color_dither.h>

LOG_MODULE_REGISTER(color_dither, CONFIG_DISPLAY_LOG_LEVEL);

enum color_dither_algorithm {
	COLOR_DITHER_ALGO_NONE,
	COLOR_DITHER_ALGO_BAYER_4X4,
	COLOR_DITHER_ALGO_BAYER_8X8,
	COLOR_DITHER_ALGO_FLOYD_STEINBERG,
	COLOR_DITHER_ALGO_ATKINSON,
};

#if defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ATKINSON)
#define COLOR_DITHER_ACTIVE_ALGORITHM COLOR_DITHER_ALGO_ATKINSON
#elif defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_FLOYD_STEINBERG)
#define COLOR_DITHER_ACTIVE_ALGORITHM COLOR_DITHER_ALGO_FLOYD_STEINBERG
#elif defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ORDERED_BAYER_8X8)
#define COLOR_DITHER_ACTIVE_ALGORITHM COLOR_DITHER_ALGO_BAYER_8X8
#elif defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ORDERED_BAYER_4X4)
#define COLOR_DITHER_ACTIVE_ALGORITHM COLOR_DITHER_ALGO_BAYER_4X4
#else
#define COLOR_DITHER_ACTIVE_ALGORITHM COLOR_DITHER_ALGO_NONE
#endif

#define COLOR_DITHER_BAYER_STEP   8
#define COLOR_DITHER_BAYER_OFFSET 8

/*
 * Standard 4x4 Bayer ordered-dithering matrix.
 *
 * The 8-point centering offset makes the matrix symmetric around zero, and a step of 8 keeps the
 * ordered dither visible enough to mix palette entries without overwhelming the source image on
 * common small display palettes.
 */
static const uint8_t bayer_4x4[4][4] = {
	{0, 8, 2, 10},
	{12, 4, 14, 6},
	{3, 11, 1, 9},
	{15, 7, 13, 5},
};

/*
 * Standard 8x8 Bayer matrix.
 *
 * A 2-point step gives it a similar component range to the 4x4 matrix while using finer spatial
 * thresholds.
 */
static const uint8_t bayer_8x8[8][8] = {
	{0, 48, 12, 60, 3, 51, 15, 63}, {32, 16, 44, 28, 35, 19, 47, 31},
	{8, 56, 4, 52, 11, 59, 7, 55},  {40, 24, 36, 20, 43, 27, 39, 23},
	{2, 50, 14, 62, 1, 49, 13, 61}, {34, 18, 46, 30, 33, 17, 45, 29},
	{10, 58, 6, 54, 9, 57, 5, 53},  {42, 26, 38, 22, 41, 25, 37, 21},
};

/* Decoded 8-bit-per-channel pixel passed between the decode, dither and quantize stages. */
struct color_dither_rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

static uint8_t apply_ordered_threshold(uint8_t component, uint8_t threshold,
				       uint8_t threshold_offset, uint8_t step)
{
	int16_t offset = ((int16_t)threshold - (int16_t)threshold_offset) * step;

	return CLAMP((int16_t)component + offset, 0, UINT8_MAX);
}

static void apply_ordered_dither(enum color_dither_algorithm algorithm, uint32_t x, uint32_t y,
				 struct color_dither_rgb *rgb)
{
	uint8_t threshold;
	uint8_t offset;
	uint8_t step;

	switch (algorithm) {
	case COLOR_DITHER_ALGO_BAYER_4X4:
		threshold = bayer_4x4[y & 0x3U][x & 0x3U];
		offset = COLOR_DITHER_BAYER_OFFSET;
		step = COLOR_DITHER_BAYER_STEP;
		break;
	case COLOR_DITHER_ALGO_BAYER_8X8:
		threshold = bayer_8x8[y & 0x7U][x & 0x7U];
		offset = 32U;
		step = 2U;
		break;
	default:
		return;
	}

	rgb->r = apply_ordered_threshold(rgb->r, threshold, offset, step);
	rgb->g = apply_ordered_threshold(rgb->g, threshold, offset, step);
	rgb->b = apply_ordered_threshold(rgb->b, threshold, offset, step);
}

static int decode_pixel_rgb(const struct display_buffer_descriptor *desc,
			    enum display_pixel_format pixel_format, const void *buf, uint32_t x,
			    uint32_t y, struct color_dither_rgb *rgb)
{
	uint32_t pixel_idx = y * desc->pitch + x;

	switch (pixel_format) {
#ifdef CONFIG_DISPLAY_COLOR_DITHER_INPUT_RGB565
	case PIXEL_FORMAT_RGB_565: {
		uint16_t pixel = sys_get_le16((const uint8_t *)buf + (pixel_idx * 2U));

		rgb->r = ((pixel >> 11) & 0x1FU) * 255U / 31U;
		rgb->g = ((pixel >> 5) & 0x3FU) * 255U / 63U;
		rgb->b = (pixel & 0x1FU) * 255U / 31U;
		return 0;
	}
#endif
#ifdef CONFIG_DISPLAY_COLOR_DITHER_INPUT_RGB888
	case PIXEL_FORMAT_RGB_888: {
		const uint8_t *buf8 = (const uint8_t *)buf + (pixel_idx * 3U);

		rgb->r = buf8[2];
		rgb->g = buf8[1];
		rgb->b = buf8[0];
		return 0;
	}
#endif
	default:
		return -ENOTSUP;
	}
}

static bool palette_entry_is_null(const struct display_palette_color *color)
{
	return color->a == 0U && color->r == 0U && color->g == 0U && color->b == 0U;
}

static int find_nearest_palette_idx(const struct display_palette_color *palette,
				    const struct color_dither_rgb *rgb)
{
	uint32_t best_distance = UINT32_MAX;
	int best_index = -1;

	for (size_t i = 0; i < CONFIG_DISPLAY_COLOR_PALETTE_MAX_SIZE; i++) {
		int16_t dr, dg, db;
		uint32_t distance;

		if (palette_entry_is_null(&palette[i])) {
			continue;
		}

		dr = (int16_t)rgb->r - (int16_t)palette[i].r;
		dg = (int16_t)rgb->g - (int16_t)palette[i].g;
		db = (int16_t)rgb->b - (int16_t)palette[i].b;
		distance = (uint32_t)(dr * dr) + (uint32_t)(dg * dg) + (uint32_t)(db * db);

		if (distance < best_distance) {
			best_distance = distance;
			best_index = (int)i;
			if (distance == 0U) {
				break;
			}
		}
	}

	return best_index;
}

static void pack_i4_nibble(uint8_t *converted_buf, size_t row_size, uint32_t x, uint32_t y,
			   uint8_t palette_idx)
{
	size_t byte_idx = (y * row_size) + (x / 2U);

	if ((x & 0x1U) == 0U) {
		converted_buf[byte_idx] = palette_idx << 4;
	} else {
		converted_buf[byte_idx] |= palette_idx & 0x0FU;
	}
}

static int quantize_to_i4(const struct display_palette_color *palette,
			  const struct color_dither_rgb *rgb, uint8_t *converted_buf,
			  size_t row_size, uint32_t x, uint32_t y)
{
	int palette_idx = find_nearest_palette_idx(palette, rgb);

	if (palette_idx < 0) {
		return -ENOTSUP;
	}

	pack_i4_nibble(converted_buf, row_size, x, y, (uint8_t)palette_idx);
	return palette_idx;
}

#if defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_FLOYD_STEINBERG) ||                              \
	defined(CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ATKINSON)

/* One neighbour to which post-quantisation error is diffused. */
struct color_dither_errdiff_tap {
	int8_t dx;
	int8_t dy;
	int8_t weight;
};

struct color_dither_errdiff_kernel {
	const struct color_dither_errdiff_tap *taps;
	uint8_t n_taps;
	uint8_t n_rows;  /* Size of the rolling error-row window (2 or 3). */
	uint8_t divisor; /* Sum of tap weights. */
};

#ifdef CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_FLOYD_STEINBERG
/* Floyd-Steinberg kernel: 7/3/5/1 over 16, two rows. */
static const struct color_dither_errdiff_tap fs_taps[] = {
	{1, 0, 7},
	{-1, 1, 3},
	{0, 1, 5},
	{1, 1, 1},
};
static const struct color_dither_errdiff_kernel fs_kernel = {
	.taps = fs_taps,
	.n_taps = ARRAY_SIZE(fs_taps),
	.n_rows = 2U,
	.divisor = 16U,
};
#endif

#ifdef CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ATKINSON
/*
 * Atkinson dithering, originally written for the Macintosh imaging stack.
 * Distributes 6/8 of the quantization error across six neighbors:
 *
 *           X   1/8  1/8
 *     1/8  1/8  1/8
 *          1/8
 *
 * The remaining 2/8 is intentionally dropped, which keeps local contrast sharper than
 * Floyd-Steinberg at the cost of slightly compressed highlights and shadows.
 */
static const struct color_dither_errdiff_tap atk_taps[] = {
	{1, 0, 1}, {2, 0, 1}, {-1, 1, 1}, {0, 1, 1}, {1, 1, 1}, {0, 2, 1},
};
static const struct color_dither_errdiff_kernel atk_kernel = {
	.taps = atk_taps,
	.n_taps = ARRAY_SIZE(atk_taps),
	.n_rows = 3U,
	.divisor = 8U,
};
#endif

static void error_accumulate(struct display_color_dither_rgb_error *error, int16_t r, int16_t g,
			     int16_t b, int16_t scale)
{
	error->r = CLAMP((int32_t)error->r + (r * scale), INT16_MIN, INT16_MAX);
	error->g = CLAMP((int32_t)error->g + (g * scale), INT16_MIN, INT16_MAX);
	error->b = CLAMP((int32_t)error->b + (b * scale), INT16_MIN, INT16_MAX);
}

static uint8_t error_apply(uint8_t component, int16_t error, int divisor)
{
	return CLAMP((int16_t)component + (error / divisor), 0, UINT8_MAX);
}

static int convert_error_diffusion(const struct display_buffer_descriptor *desc,
				   enum display_pixel_format pixel_format, const void *buf,
				   const struct display_palette_color *palette,
				   struct display_color_dither_state *state,
				   const struct color_dither_errdiff_kernel *kernel)
{
	size_t row_size = DIV_ROUND_UP(desc->width, 2U);
	size_t head = 0U;

	if (kernel->n_rows == 0U || kernel->n_rows > ARRAY_SIZE(state->err_rows)) {
		return -EINVAL;
	}
	if (state->err_row_len < desc->width + (kernel->n_rows - 1U)) {
		return -EINVAL;
	}
	for (size_t i = 0; i < kernel->n_rows; i++) {
		if (state->err_rows[i] == NULL) {
			return -EINVAL;
		}
		memset(state->err_rows[i], 0, state->err_row_len * sizeof(*state->err_rows[i]));
	}

	for (uint32_t y = 0U; y < desc->height; y++) {
		struct display_color_dither_rgb_error *current_row = state->err_rows[head];

		for (uint32_t x = 0U; x < desc->width; x++) {
			const struct display_palette_color *color;
			struct color_dither_rgb rgb;
			int16_t er, eg, eb;
			int palette_idx;

			if (decode_pixel_rgb(desc, pixel_format, buf, x, y, &rgb) != 0) {
				return -ENOTSUP;
			}

			rgb.r = error_apply(rgb.r, current_row[x].r, kernel->divisor);
			rgb.g = error_apply(rgb.g, current_row[x].g, kernel->divisor);
			rgb.b = error_apply(rgb.b, current_row[x].b, kernel->divisor);
			palette_idx =
				quantize_to_i4(palette, &rgb, state->converted_buf, row_size, x, y);
			if (palette_idx < 0) {
				return -ENOTSUP;
			}

			color = &palette[palette_idx];
			er = (int16_t)rgb.r - (int16_t)color->r;
			eg = (int16_t)rgb.g - (int16_t)color->g;
			eb = (int16_t)rgb.b - (int16_t)color->b;

			for (size_t i = 0; i < kernel->n_taps; i++) {
				int32_t nx = (int32_t)x + kernel->taps[i].dx;
				size_t dy;
				size_t row_idx;

				if (nx < 0 || (uint32_t)nx >= desc->width) {
					continue;
				}
				if (kernel->taps[i].dy < 0) {
					continue;
				}
				dy = (size_t)kernel->taps[i].dy;
				if (dy >= kernel->n_rows) {
					continue;
				}
				if (y + dy >= desc->height) {
					continue;
				}

				/* head + dy is in [0, 2*n_rows - 2], so at most one
				 * subtraction wraps it back into range. Avoids a runtime
				 * modulo in the inner loop.
				 */
				row_idx = head + dy;
				if (row_idx >= kernel->n_rows) {
					row_idx -= kernel->n_rows;
				}
				error_accumulate(&state->err_rows[row_idx][nx], er, eg, eb,
						 kernel->taps[i].weight);
			}
		}

		/* Advance the rolling window: the row we just consumed becomes the new
		 * bottom and gets zeroed out for reuse.
		 */
		memset(state->err_rows[head], 0,
		       state->err_row_len * sizeof(*state->err_rows[head]));
		head++;
		if (head == kernel->n_rows) {
			head = 0U;
		}
	}

	return 0;
}

#endif /* FLOYD_STEINBERG || ATKINSON */

static int convert_ordered(const struct display_buffer_descriptor *desc,
			   enum display_pixel_format pixel_format, const void *buf,
			   const struct display_palette_color *palette, uint8_t *converted_buf,
			   enum color_dither_algorithm algorithm)
{
	size_t row_size = DIV_ROUND_UP(desc->width, 2U);

	for (uint32_t y = 0U; y < desc->height; y++) {
		for (uint32_t x = 0U; x < desc->width; x++) {
			struct color_dither_rgb rgb;

			if (decode_pixel_rgb(desc, pixel_format, buf, x, y, &rgb) != 0) {
				return -ENOTSUP;
			}

			apply_ordered_dither(algorithm, x, y, &rgb);
			if (quantize_to_i4(palette, &rgb, converted_buf, row_size, x, y) < 0) {
				return -ENOTSUP;
			}
		}
	}

	return 0;
}

#define COLOR_DITHER_SUPPORTED_FORMATS                                                             \
	((IS_ENABLED(CONFIG_DISPLAY_COLOR_DITHER_INPUT_RGB565) ? PIXEL_FORMAT_RGB_565 : 0) |       \
	 (IS_ENABLED(CONFIG_DISPLAY_COLOR_DITHER_INPUT_RGB888) ? PIXEL_FORMAT_RGB_888 : 0))

int display_color_dither_set_input_format(struct display_color_dither_state *state,
					  enum display_pixel_format pf)
{
	/* I_4 is the native target format: select the driver's passthrough path with no
	 * conversion. It is intentionally not part of COLOR_DITHER_SUPPORTED_FORMATS (those are
	 * the RGB inputs the helper can convert), so it must be handled before that check.
	 */
	if (pf == PIXEL_FORMAT_I_4) {
		if (state != NULL) {
			state->input_format = pf;
		}
		return 0;
	}

	if (state == NULL) {
		return -ENOTSUP;
	}

	if (state->converted_buf == NULL) {
		return -ENOTSUP;
	}

	if (!IS_POWER_OF_TWO(pf) || (pf & COLOR_DITHER_SUPPORTED_FORMATS) == 0U) {
		return -ENOTSUP;
	}

	state->input_format = pf;
	return 0;
}

bool display_color_dither_is_active(const struct display_color_dither_state *state,
				    enum display_pixel_format current_pixel_format)
{
	return state != NULL && state->converted_buf != NULL &&
	       state->input_format != PIXEL_FORMAT_I_4 &&
	       state->input_format == current_pixel_format;
}

void display_color_dither_patch_caps(struct display_color_dither_state *state,
				     struct display_capabilities *caps)
{
	if (state == NULL || state->converted_buf == NULL) {
		return;
	}

	caps->supported_pixel_formats |= COLOR_DITHER_SUPPORTED_FORMATS;

	/*
	 * Only override the driver-reported current format when the helper is actively converting.
	 * For I_4 input the driver speaks for itself.
	 */
	if (state->input_format != PIXEL_FORMAT_I_4) {
		caps->current_pixel_format = state->input_format;
	}
}

int display_color_dither_prepare(const struct device *dev, struct display_color_dither_state *state,
				 const struct display_buffer_descriptor **desc_in_out,
				 const void **buf_in_out, struct display_buffer_descriptor *scratch)
{
	if (state == NULL || state->converted_buf == NULL ||
	    state->input_format == PIXEL_FORMAT_I_4) {
		return 0;
	}

	const struct display_buffer_descriptor *in_desc = *desc_in_out;

	if (in_desc->width > in_desc->pitch) {
		return -EINVAL;
	}

	size_t input_pixel_bits = DISPLAY_BITS_PER_PIXEL(state->input_format);

	if (input_pixel_bits == 0U) {
		return -ENOTSUP;
	}

	if (in_desc->buf_size < (in_desc->pitch * in_desc->height * input_pixel_bits) / 8U) {
		return -EINVAL;
	}

	size_t required_size = COLOR_DITHER_I4_BYTES(in_desc->width, in_desc->height);

	if (state->converted_buf_size < required_size) {
		LOG_ERR("converted buffer too small (%zu < %zu)", state->converted_buf_size,
			required_size);
		return -ENOMEM;
	}

	struct display_capabilities caps;
	int ret;

	display_get_capabilities(dev, &caps);
	memset(state->converted_buf, 0, required_size);

	switch (COLOR_DITHER_ACTIVE_ALGORITHM) {
	case COLOR_DITHER_ALGO_NONE:
	case COLOR_DITHER_ALGO_BAYER_4X4:
	case COLOR_DITHER_ALGO_BAYER_8X8:
		ret = convert_ordered(in_desc, state->input_format, *buf_in_out, caps.color_palette,
				      state->converted_buf, COLOR_DITHER_ACTIVE_ALGORITHM);
		break;
#ifdef CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_FLOYD_STEINBERG
	case COLOR_DITHER_ALGO_FLOYD_STEINBERG:
		ret = convert_error_diffusion(in_desc, state->input_format, *buf_in_out,
					      caps.color_palette, state, &fs_kernel);
		break;
#endif
#ifdef CONFIG_DISPLAY_COLOR_DITHER_ALGORITHM_ATKINSON
	case COLOR_DITHER_ALGO_ATKINSON:
		ret = convert_error_diffusion(in_desc, state->input_format, *buf_in_out,
					      caps.color_palette, state, &atk_kernel);
		break;
#endif
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		return ret;
	}

	*scratch = *in_desc;
	scratch->buf_size = required_size;
	scratch->pitch = in_desc->width;

	*desc_in_out = scratch;
	*buf_in_out = state->converted_buf;
	return 0;
}
