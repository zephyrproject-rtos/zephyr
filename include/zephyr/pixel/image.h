/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_IMAGE_H
#define ZEPHYR_INCLUDE_PIXEL_IMAGE_H

#include <zephyr/sys/slist.h>
#include <zephyr/pixel/operation.h>
#include <zephyr/pixel/kernel.h>

/**
 * @brief Represent the image currently being processed
 */
struct pixel_image {
	sys_slist_t operations;
	/** Current width of the image */
	uint16_t width;
	/** Current height of the image */
	uint16_t height;
	/** Current pixel format of the image as a four character code (fourcc) */
	uint32_t fourcc;
	/** Input or output buffer used with the conversion */
	uint8_t *buffer;
	/** Size of the input or output buffer */
	size_t size;
	/** In case an error occurred, this is set to a matching error code */
	int err;
};

/**
 * @brief Initialize an image from a memory buffer.
 *
 * @param img Image to initialize.
 * @param buffer Memory containinig input image data to process.
 * @param size Total available size in the buffer, can be bigger/smaller than full width x height.
 * @param width Width of the complete image in pixels.
 * @param height Height of the complete image in pixels.
 * @param fourcc Format of data in the buffer as a four-character-code.
 */
void pixel_image_from_buffer(struct pixel_image *img, uint8_t *buffer, size_t size,
			     uint16_t width, uint16_t height, uint32_t fourcc);

/**
 * @brief Initialize an image from a video buffer.
 *
 * @param img Image to initialize.
 * @param vbuf Video buffer that contains the image data to process.
 * @param fmt Format of that video buffer.
 */
void pixel_image_from_vbuf(struct pixel_image *img, struct video_buffer *vbuf,
			   struct video_format *fmt);

/**
 * @brief Initialize an image from a memory buffer.
 *
 * @param img Image being processed.
 * @param buffer Memory that receives the image data.
 * @param size Size of the buffer.
 * @return 0 on success
 */
int pixel_image_to_buffer(struct pixel_image *img, uint8_t *buffer, size_t size);

/**
 * @brief Initialize an image from a memory buffer.
 *
 * @param img Image being processed.
 * @param vbuf Video buffer that receives the image data.
 * @return 0 on success
 */
int pixel_image_to_vbuf(struct pixel_image *img, struct video_buffer *vbuf);

/**
 * @brief Convert an image to a new pixel format.
 *
 * An operation is added to convert the image to a new pixel format.
 * If the operation to convert the image from the current format to a new format does not exist,
 * then the error flag is set, which can be accessed as @c img->err.
 *
 * In some cases, converting between two formats requires an intermediate conversion to RGB24.
 *
 * @param img Image to convert.
 * @param new_format A four-character-code (FOURCC) as defined by @c <zephyr/drivers/video.h>.
 */
int pixel_image_convert(struct pixel_image *img, uint32_t new_format);

/**
 * @brief Convert an image from a bayer array format to RGB24.
 *
 * An operation is added to convert the image to RGB24 using the specified window size, such
 * as 2x2 or 3x3.
 *
 * @note It is also possible to use @ref pixel_image_convert to convert from bayer to RGB24 but
 *       this does not allow to select the window size.
 *
 * @param img Image to convert.
 * @param window_size The window size for convnerting, usually 2 (faster) or 3 (higher quality).
 */
int pixel_image_debayer(struct pixel_image *img, uint32_t window_size);

/**
 * @brief Resize an image.
 *
 * An operation is added to change the image size. The aspect ratio is not preserved and the output
 * image size is exactly the same as requested.
 *
 * @param img Image to convert.
 * @param width The new width in pixels.
 * @param height The new height in pixels.
 */
int pixel_image_resize(struct pixel_image *img, uint16_t width, uint16_t height);

/**
 * @brief Apply a kernel operation on an image.
 *
 * Kernel operations are working on small blocks of typically 3x3 or 5x5 pixels, repeated over the
 * entire image to apply a desired effect on an image.
 *
 * @param img Image to convert.
 * @param kernel_type The type of kernel to apply as defined in <zephyr/pixel/kernel.h>
 * @param kernel_size The size of the kernel operaiton, usually 3 or 5.
 */
int pixel_image_kernel(struct pixel_image *img, uint32_t kernel_type, int kernel_size);

/**
 * @brief Print an image using higher quality TRUECOLOR terminal escape codes.
 *
 * @param img Image to print.
 */
void pixel_image_print_truecolor(struct pixel_image *img);

/**
 * @brief Print an image using higher speed 256COLOR terminal escape codes.
 *
 * @param img Image to print.
 */
void pixel_image_print_256color(struct pixel_image *img);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Add a operation processing step to an image.
 *
 * @note This is a low-level function only needed to implement new operations.
 *
 * The operation step will not be processed immediately, but rather added to a linked list of
 * operations that will be performed at runtime.
 *
 * @param img Image to which add a processing step.
 * @param template Stream processing step to apply to the image.
 * @param buffer_size Size of the input buffer to allocate for this operation.
 * @param threshold Minimum number of bytes the operation needs to run one cycle.
 * @return 0 on success
 */
int pixel_image_add_operation(struct pixel_image *img, const struct pixel_operation *template,
			      size_t buffer_size, size_t threshold);

/**
 * @brief Add a operation processing step to an image for uncompressed input data.
 *
 * @note This is a low-level function only needed to implement new operations.
 *
 * The operation step will not be processed immediately, but rather added to a linked list of
 * operations that will be performed at runtime.
 *
 * Details such as buffer size or threshold value are deduced from the stream.
 *
 * @param img Image to which add a processing step.
 * @param template Stream processing step to apply to the image.
 */
int pixel_image_add_uncompressed(struct pixel_image *img, const struct pixel_operation *template);

/**
 * @brief Perform all the processing added to the
 *
 * @note This is a low-level function only needed to implement new operations.
 *
 * This is where all the image processing happens. The processing steps are not executed while they
 * are added to the pipeline, but only while this function is called.
 *
 * @param img Image to which one or multiple processing steps were added.
 * @return 0 on success.
 */
int pixel_image_process(struct pixel_image *img);

/**
 * @brief Perform all the processing added to the
 *
 * @note This is a low-level function only needed to implement new operations.
 *
 * This is where all the image processing happens. The processing steps are not executed while they
 * are added to the pipeline, but only while this function is called.
 *
 * @param img Image to which one or multiple processing steps were added.
 * @return 0 on success.
 */
int pixel_image_error(struct pixel_image *img, int err);

/** @endcond */

#endif /* ZEPHYR_INCLUDE_PIXEL_IMAGE_H */
