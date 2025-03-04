/*
 * Copyir (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/pixel/image.h>
#include <zephyr/pixel/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pixel_kernel, CONFIG_PIXEL_LOG_LEVEL);

int pixel_image_kernel(struct pixel_image *img, uint32_t kernel_type, int kernel_size)
{
	const struct pixel_operation *op = NULL;

	STRUCT_SECTION_FOREACH_ALTERNATE(pixel_kernel, pixel_operation, tmp) {
		if (tmp->fourcc_in == img->fourcc && tmp->type == kernel_type &&
		    kernel_size == tmp->window_size) {
			op = tmp;
			break;
		}
	}

	if (op == NULL) {
		LOG_ERR("Kernel operation %u of size %ux%u on %s data not found",
			kernel_type, kernel_size, kernel_size, VIDEO_FOURCC_TO_STR(img->fourcc));
		return pixel_image_error(img, -ENOSYS);
	}

	return pixel_image_add_uncompressed(img, op);
}

/* Function that processes a 3x3 or 5x5 pixel block described by line buffers and column indexes */
typedef void kernel_3x3_t(const uint8_t *in[3], int i0, int i1, int i2,
			  uint8_t *out, int o0, uint16_t base, const uint16_t *kernel);
typedef void kernel_5x5_t(const uint8_t *in[3], int i0, int i1, int i2, int i3, int i4,
			  uint8_t *out, int o0, uint16_t base, const uint16_t *kernel);

/* Function that repeats a 3x3 or 5x5 block operation to each channel of a pixel format */
typedef void pixfmt_3x3_t(const uint8_t *in[3], int i0, int i1, int i2,
			  uint8_t *out, int o0, uint16_t base, kernel_3x3_t *line_fn,
			  const uint16_t *kernel);
typedef void pixfmt_5x5_t(const uint8_t *in[5], int i0, int i1, int i2, int i3, int i4,
			  uint8_t *out, int o0, uint16_t base, kernel_5x5_t *line_fn,
			  const uint16_t *kernel);

/* Function that repeats a 3x3 or 5x5 kernel operation over an entire line line */
typedef void line_3x3_t(const uint8_t *in[3], uint8_t *out, uint16_t width);
typedef void line_5x5_t(const uint8_t *in[5], uint8_t *out, uint16_t width);

/*
 * Convolution kernels: multiply a grid of coefficient with the input data and um them to produce
 * one output value.
 */

static void pixel_convolve_3x3(const uint8_t *in[3], int i0, int i1, int i2,
			       uint8_t *out, int o0, uint16_t base, const uint16_t *kernel)
{
	int16_t result = 0;
	int k = 0;

	/* Apply the coefficients on 3 rows */
	for (int h = 0; h < 3; h++) {
		/* Apply the coefficients on 5 columns */
		result += in[h][base + i0] * kernel[k++]; /* line h column 0 */
		result += in[h][base + i1] * kernel[k++]; /* line h column 1 */
		result += in[h][base + i2] * kernel[k++]; /* line h column 2 */
	}

	/* Store the scaled-down output */
	out[base + o0] = result >> kernel[k];
}

static void pixel_convolve_5x5(const uint8_t *in[5], int i0, int i1, int i2, int i3, int i4,
			       uint8_t *out, int o0, uint16_t base, const uint16_t *kernel)
{
	int16_t result = 0;
	int k = 0;

	/* Apply the coefficients on 5 rows */
	for (int h = 0; h < 5; h++) {
		/* Apply the coefficients on 5 columns */
		result += in[h][base + i0] * kernel[k++]; /* line h column 0 */
		result += in[h][base + i1] * kernel[k++]; /* line h column 1 */
		result += in[h][base + i2] * kernel[k++]; /* line h column 2 */
		result += in[h][base + i3] * kernel[k++]; /* line h column 3 */
		result += in[h][base + i4] * kernel[k++]; /* line h column 4 */
	}

	/* Store the scaled-down output */
	out[base + o0] = result >> kernel[k];
}

/*
 * Median kernels: find the median value of the input block and send it as output. The effect is to
 * denoise the input image while preserving sharpness of the large color regions.
 */

static inline uint8_t pixel_median(const uint8_t **in, int *idx, uint8_t size)
{
	uint8_t pivot_bot = 0x00;
	uint8_t pivot_top = 0xff;
	uint8_t num_higher;
	int16_t median;

	/* Binary-search of the appropriate median value, 8 steps for 8-bit depth */
	for (int i = 0; i < 8; i++) {
		num_higher = 0;
		median = (pivot_top + pivot_bot) / 2;

		for (uint16_t h = 0; h < size; h++) {
			for (uint16_t w = 0; w < size; w++) {
				num_higher += in[h][idx[w]] > median; /* line h column w */
			}
		}

		if (num_higher > size * size / 2) {
			pivot_bot = median;
		} else if (num_higher < size * size / 2) {
			pivot_top = median;
		}
	}

	/* Output the median value */
	return (pivot_top + pivot_bot) / 2;
}

static void pixel_median_3x3(const uint8_t *in[3], int i0, int i1, int i2,
			     uint8_t *out, int o0, uint16_t base, const uint16_t *unused)
{
	int idx[] = {base + i0, base + i1, base + i2};

	out[base + o0] = pixel_median(in, idx, 3);
}

static void pixel_median_5x5(const uint8_t *in[5], int i0, int i1, int i2, int i3, int i4,
			     uint8_t *out, int o0, uint16_t base, const uint16_t *unused)
{
	int idx[] = {base + i0, base + i1, base + i2, base + i3, base + i4};

	out[base + o0] = pixel_median(in, idx, 5);
}

/*
 * Convert pixel offsets into byte offset, and repeat a kernel function for every channel of a
 * pixel format.
 */

static void pixel_kernel_rgb24_3x3(const uint8_t *in[3], int i0, int i1, int i2,
				   uint8_t *out, int o0, uint16_t base, kernel_3x3_t *line_fn,
				   const uint16_t *kernel)
{
	i0 *= 3, i1 *= 3, i2 *= 3, o0 *= 3, base *= 3;
	line_fn(in, i0, i1, i2, out, o0, base + 0, kernel); /* R */
	line_fn(in, i0, i1, i2, out, o0, base + 1, kernel); /* G */
	line_fn(in, i0, i1, i2, out, o0, base + 2, kernel); /* B */
}

static void pixel_kernel_rgb24_5x5(const uint8_t *in[5], int i0, int i1, int i2, int i3, int i4,
				   uint8_t *out, int o0, uint16_t base, kernel_5x5_t *line_fn,
				   const uint16_t *kernel)
{
	i0 *= 3, i1 *= 3, i2 *= 3, i3 *= 3, i4 *= 3, o0 *= 3, base *= 3;
	line_fn(in, i0, i1, i2, i3, i4, out, o0, base + 0, kernel); /* R */
	line_fn(in, i0, i1, i2, i3, i4, out, o0, base + 1, kernel); /* G */
	line_fn(in, i0, i1, i2, i3, i4, out, o0, base + 2, kernel); /* B */
}

/*
 * Portable/default C implementation of line processing functions. They are inlined into
 * line-conversion functions at the bottom of this file declared as __weak.
 */

static inline void pixel_kernel_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width,
					 pixfmt_3x3_t *pixfmt_fn, kernel_3x3_t *line_fn,
					 const uint16_t *kernel)
{
	uint16_t w = 0;

	/* Edge case on first two columns */
	pixfmt_fn(in, 0, 0, 1, out, 0, w + 0, line_fn, kernel);

	/* process the entire line except the first two and last two columns (edge cases) */
	for (w = 0; w + 3 <= width; w++) {
		pixfmt_fn(in, 0, 1, 2, out, 1, w, line_fn, kernel);
	}

	/* Edge case on last two columns */
	pixfmt_fn(in, 0, 1, 1, out, 1, w, line_fn, kernel);
}

static inline void pixel_kernel_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width,
					 pixfmt_5x5_t *pixfmt_fn, kernel_5x5_t *line_fn,
					 const uint16_t *kernel)
{
	uint16_t w = 0;

	/* Edge case on first two columns, repeat the left column to fill the blank */
	pixfmt_fn(in, 0, 0, 0, 1, 2, out, 0, w, line_fn, kernel);
	pixfmt_fn(in, 0, 0, 1, 2, 3, out, 1, w, line_fn, kernel);

	/* process the entire line except the first two and last two columns (edge cases) */
	for (w = 0; w + 5 <= width; w++) {
		pixfmt_fn(in, 0, 1, 2, 3, 4, out, 2, w, line_fn, kernel);
	}

	/* Edge case on last two columns, repeat the right column to fill the blank */
	pixfmt_fn(in, 0, 1, 2, 3, 3, out, 2, w, line_fn, kernel);
	pixfmt_fn(in, 1, 2, 3, 3, 3, out, 3, w, line_fn, kernel);
}

/*
 * Call a line-processing function on every line, handling the edge-cases on first line and last
 * line by repeating the lines at the edge to fill the gaps.
 */

void pixel_kernel_3x3_op(struct pixel_operation *op)
{
	uint16_t prev_line_offset = op->line_offset;
	line_3x3_t *line_fn = op->arg;
	const uint8_t *in[] = {
		pixel_operation_get_input_line(op),
		pixel_operation_peek_input_line(op),
		pixel_operation_peek_input_line(op),
	};

	__ASSERT_NO_MSG(op->width >= 3);
	__ASSERT_NO_MSG(op->height >= 3);

	/* Allow overflowing before the top by repeating the first line */
	if (prev_line_offset == 0) {
		const uint8_t *top[] = {in[0], in[0], in[1]};

		line_fn(top, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	}

	/* Process one more line */
	line_fn(in, pixel_operation_get_output_line(op), op->width);
	pixel_operation_done(op);

	/* Allow overflowing after the bottom by repeating the last line */
	if (prev_line_offset + 3 >= op->height) {
		const uint8_t *bot[] = {in[1], in[2], in[2]};

		line_fn(bot, pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		/* Flush the remaining lines that were used for lookahead context */
		pixel_operation_get_input_line(op);
		pixel_operation_get_input_line(op);
	}
}

void pixel_kernel_5x5_op(struct pixel_operation *op)
{
	uint16_t prev_line_offset = op->line_offset;
	line_5x5_t *line_fn = op->arg;
	const uint8_t *in[] = {
		pixel_operation_get_input_line(op),
		pixel_operation_peek_input_line(op),
		pixel_operation_peek_input_line(op),
		pixel_operation_peek_input_line(op),
		pixel_operation_peek_input_line(op),
	};

	__ASSERT_NO_MSG(op->width >= 5);
	__ASSERT_NO_MSG(op->height >= 5);

	/* Allow overflowing before the top by repeating the first line */
	if (prev_line_offset == 0) {
		const uint8_t *top[] = {in[0], in[0], in[0], in[1], in[2], in[3]};

		line_fn(&top[0], pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		line_fn(&top[1], pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);
	}

	/* Process one more line */
	line_fn(in, pixel_operation_get_output_line(op), op->width);
	pixel_operation_done(op);

	/* Allow overflowing after the bottom by repeating the last line */
	if (prev_line_offset + 5 >= op->height) {
		const uint8_t *bot[] = {in[1], in[2], in[3], in[4], in[4], in[4]};

		line_fn(&bot[0], pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		line_fn(&bot[1], pixel_operation_get_output_line(op), op->width);
		pixel_operation_done(op);

		/* Flush the remaining lines that were used for lookahead context */
		pixel_operation_get_input_line(op);
		pixel_operation_get_input_line(op);
		pixel_operation_get_input_line(op);
		pixel_operation_get_input_line(op);
	}
}

/*
 * Declaration of convolution kernels, with the line-processing functions declared as __weak to
 * allow them to be replaced with optimized versions
 */

static const int16_t pixel_identity_3x3[] = {
	0, 0, 0,
	0, 1, 0,
	0, 0, 0, 0
};

__weak void pixel_identity_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width)
{
	pixel_kernel_3x3(in, out, width, pixel_kernel_rgb24_3x3, pixel_convolve_3x3,
			 pixel_identity_3x3);
}
PIXEL_DEFINE_KERNEL_3X3_OPERATION(pixel_identity_rgb24_3x3,
				  PIXEL_KERNEL_IDENTITY, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_identity_5x5[] = {
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 1, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0
};

__weak void pixel_identity_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width)
{
	pixel_kernel_5x5(in, out, width, pixel_kernel_rgb24_5x5, pixel_convolve_5x5,
			 pixel_identity_5x5);
}
PIXEL_DEFINE_KERNEL_5X5_OPERATION(pixel_identity_rgb24_5x5,
				  PIXEL_KERNEL_IDENTITY, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_edgedetect_3x3[] = {
	-1, -1, -1,
	-1,  8, -1,
	-1, -1, -1, 0
};

__weak void pixel_edgedetect_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width)
{
	pixel_kernel_3x3(in, out, width, pixel_kernel_rgb24_3x3, pixel_convolve_3x3,
			      pixel_edgedetect_3x3);
}
PIXEL_DEFINE_KERNEL_3X3_OPERATION(pixel_edgedetect_rgb24_3x3,
				  PIXEL_KERNEL_EDGE_DETECT, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_gaussianblur_3x3[] = {
	1, 2, 1,
	2, 4, 2,
	1, 2, 1, 4
};

__weak void pixel_gaussianblur_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width)
{
	pixel_kernel_3x3(in, out, width, pixel_kernel_rgb24_3x3, pixel_convolve_3x3,
			      pixel_gaussianblur_3x3);
}
PIXEL_DEFINE_KERNEL_3X3_OPERATION(pixel_gaussianblur_rgb24_3x3,
				  PIXEL_KERNEL_GAUSSIAN_BLUR, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_gaussianblur_5x5[] = {
	1,  4,  6,  4, 1,
	4, 16, 24, 16, 4,
	6, 24, 36, 24, 6,
	4, 16, 24, 16, 4,
	1,  4,  6,  4, 1, 8
};

__weak void pixel_gaussianblur_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width)
{
	pixel_kernel_5x5(in, out, width, pixel_kernel_rgb24_5x5, pixel_convolve_5x5,
			 pixel_gaussianblur_5x5);
}
PIXEL_DEFINE_KERNEL_5X5_OPERATION(pixel_gaussianblur_rgb24_5x5,
				  PIXEL_KERNEL_GAUSSIAN_BLUR, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_sharpen_3x3[] = {
	 0, -1,  0,
	-1,  5, -1,
	 0, -1,  0, 0
};

__weak void pixel_sharpen_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width)
{
	pixel_kernel_3x3(in, out, width, pixel_kernel_rgb24_3x3, pixel_convolve_3x3,
			      pixel_sharpen_3x3);
}
PIXEL_DEFINE_KERNEL_3X3_OPERATION(pixel_sharpen_rgb24_3x3,
				  PIXEL_KERNEL_SHARPEN, VIDEO_PIX_FMT_RGB24);

static const int16_t pixel_unsharp_5x5[] = {
	-1,  -4,  -6,  -4, -1,
	-4, -16, -24, -16, -4,
	-6, -24, 476, -24, -6,
	-4, -16, -24, -16, -4,
	-1,  -4,  -6,  -4, -1, 8
};

__weak void pixel_sharpen_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width)
{
	pixel_kernel_5x5(in, out, width, pixel_kernel_rgb24_5x5, pixel_convolve_5x5,
			 pixel_unsharp_5x5);
}
PIXEL_DEFINE_KERNEL_5X5_OPERATION(pixel_sharpen_rgb24_5x5,
				  PIXEL_KERNEL_SHARPEN, VIDEO_PIX_FMT_RGB24);

/*
 * Declaration of median kernels, with the line-processing functions declared as __weak to
 * allow them to be replaced with optimized versions
 */

__weak void pixel_median_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width)
{
	pixel_kernel_5x5(in, out, width, pixel_kernel_rgb24_5x5, pixel_median_5x5, NULL);
}

PIXEL_DEFINE_KERNEL_5X5_OPERATION(pixel_median_rgb24_5x5,
				  PIXEL_KERNEL_DENOISE, VIDEO_PIX_FMT_RGB24);

__weak void pixel_median_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width)
{
	pixel_kernel_3x3(in, out, width, pixel_kernel_rgb24_3x3, pixel_median_3x3, NULL);
}
PIXEL_DEFINE_KERNEL_3X3_OPERATION(pixel_median_rgb24_3x3,
				  PIXEL_KERNEL_DENOISE, VIDEO_PIX_FMT_RGB24);
