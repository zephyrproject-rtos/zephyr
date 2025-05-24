/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_KERNEL_H_
#define ZEPHYR_INCLUDE_PIXEL_KERNEL_H_

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/pixel/operation.h>

/**
 * @brief Define a new 5x5 kernel conversion operation.
 *
 * @param _fn Function converting 5 input lines into 1 output line.
 * @param _type Kernel operation type
 * @param _fourcc The input format for that operation.
 */
#define PIXEL_DEFINE_KERNEL_5X5_OPERATION(_fn, _type, _fourcc)                                     \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(pixel_kernel, pixel_operation,              \
						       _fn##_op) = {                               \
		.name = #_fn,                                                                      \
		.fourcc_in = _fourcc,                                                              \
		.fourcc_out = _fourcc,                                                             \
		.window_size = 5,                                                                  \
		.run = pixel_kernel_5x5_op,                                                        \
		.arg = _fn,                                                                        \
		.type = _type,                                                                     \
	}

/**
 * @brief Define a new 3x3 kernel conversion operation.
 *
 * @param _fn Function converting 3 input lines into 1 output line.
 * @param _type Kernel operation type
 * @param _fourcc The input format for that operation.
 */
#define PIXEL_DEFINE_KERNEL_3X3_OPERATION(_fn, _type, _fourcc)                                     \
	static const STRUCT_SECTION_ITERABLE_ALTERNATE(pixel_kernel, pixel_operation,              \
						       _fn##_op) = {                               \
		.name = #_fn,                                                                      \
		.fourcc_in = _fourcc,                                                              \
		.fourcc_out = _fourcc,                                                             \
		.window_size = 3,                                                                  \
		.run = pixel_kernel_3x3_op,                                                        \
		.arg = _fn,                                                                        \
		.type = _type,                                                                     \
	}

/**
 * @brief Helper to turn a 5x5 kernel conversion function into an operation.
 *
 * The line conversion function is free to perform any processing on the input lines and expected
 * to produce one output line.
 *
 * The line conversion function is to be provided in @c op->arg.
 *
 * @param op Current operation in progress.
 */
void pixel_kernel_5x5_op(struct pixel_operation *op);

/**
 * @brief Helper to turn a 3x3 kernel conversion function into an operation.
 * @copydetails pixel_kernel_5x5_op()
 */
void pixel_kernel_3x3_op(struct pixel_operation *op);

/**
 * Available kernel operations to apply to the image.
 */
enum pixel_kernel_type {
	/** Identity kernel: no change, the input is the same as the output */
	PIXEL_KERNEL_IDENTITY,
	/** Edge detection kernel: keep only an outline of the edges */
	PIXEL_KERNEL_EDGE_DETECT,
	/** Gaussian blur kernel: apply a blur onto an image following a Gaussian curve */
	PIXEL_KERNEL_GAUSSIAN_BLUR,
	/** Sharpen kernel: accentuate the edges, making the image look less blurry */
	PIXEL_KERNEL_SHARPEN,
	/** Denoise kernel: remove the parasitic image noise using the local median value */
	PIXEL_KERNEL_DENOISE,
};

/**
 * @brief Apply a 3x3 identity kernel to an RGB24 input window and produce one RGB24 line.
 *
 * @param in Array of input line buffers to convert.
 * @param out Pointer to the output line converted.
 * @param width Width of the input and output lines in pixels.
 */
void pixel_identity_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 5x5 identity kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_identity_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 sharpen kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_sharpen_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 5x5 unsharp kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_sharpen_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 edge detection kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_edgedetect_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 gaussian blur kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_gaussianblur_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 3x3 median denoise kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_median_rgb24_3x3(const uint8_t *in[3], uint8_t *out, uint16_t width);
/**
 * @brief Apply a 5x5 median denoise kernel to an RGB24 input window and produce one RGB24 line.
 * @copydetails pixel_identity_rgb24_3x3()
 */
void pixel_median_rgb24_5x5(const uint8_t *in[5], uint8_t *out, uint16_t width);

#endif /* ZEPHYR_INCLUDE_PIXEL_KERNEL_H_ */
