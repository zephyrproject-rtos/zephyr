/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PIXEL_STATS_H
#define ZEPHYR_INCLUDE_PIXEL_STATS_H

#include <stdint.h>

/**
 * @brief Collect red, green, blue channel averages of all pixels in an RGB24 frame.
 *
 * @param buf Buffer of pixels in RGB24 format (3 bytes per pixel) to collect the statistics from..
 * @param size Size of this input buffer.
 * @param rgb24avg The channel averages stored as an RGB24 pixel.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rgb24frame_to_rgb24avg(const uint8_t *buf, size_t size, uint8_t rgb24avg[3],
				  uint16_t nval);

/**
 * @brief Collect red, green, blue channel averages of all pixels in an RGGB8 frame.
 *
 * @param buf Buffer of pixels in bayer format (1 byte per pixel) to collect the statistics from..
 * @param size Size of this input buffer.
 * @param width Width of the lines in number of pixels.
 * @param rgb24avg The channel averages stored as an RGB24 pixel.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rggb8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval);
/**
 * @brief Collect red, green, blue channel averages of all pixels in an BGGR8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24avg()
 */
void pixel_bggr8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval);
/**
 * @brief Collect red, green, blue channel averages of all pixels in an GBRG8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24avg()
 */
void pixel_gbrg8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval);
/**
 * @brief Collect red, green, blue channel averages of all pixels in an GRBG8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24avg()
 */
void pixel_grbg8frame_to_rgb24avg(const uint8_t *buf, size_t size, uint16_t width,
				  uint8_t rgb24avg[3], uint16_t nval);

/**
 * @brief Collect an histogram for each of the red, green, blue channels of an RGB24 frame.
 *
 * @param buf Buffer of pixels in RGB24 format (3 bytes per pixel) to collect the statistics from..
 * @param buf_size Size of this input buffer.
 * @param rgb24hist Buffer storing 3 histograms one after the other, for the R, G, B channels.
 * @param hist_size Total number of buckets in the histogram, all channels included.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rgb24frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t *rgb24hist,
				   size_t hist_size, uint16_t nval);

/**
 * @brief Collect an histogram for each of the red, green, blue channels of an RGGB8 frame.
 *
 * @param buf Buffer of pixels to collect the statistics from..
 * @param buf_size Size of this input buffer.
 * @param width Width of the lines in number of pixels.
 * @param rgb24hist Buffer storing 3 histograms one after the other, for the R, G, B channels.
 * @param hist_size Total number of buckets in the histogram, all channels included.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rggb8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for each of the red, green, blue channels of GBRG8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24hist()
 */
void pixel_gbrg8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for each of the red, green, blue channels of BGGR8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24hist()
 */
void pixel_bggr8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for each of the red, green, blue channels of GRBG8 frame.
 * @copydetails pixel_rggb8frame_to_rgb24hist()
 */
void pixel_grbg8frame_to_rgb24hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				   uint16_t *rgb24hist, size_t hist_size, uint16_t nval);

/**
 * @brief Collect an histogram for the Y channel, obtained from the pixel values of the image.
 *
 * @param buf Buffer of pixels in RGB24 format (3 bytes per pixel) to collect the statistics from..
 * @param buf_size Size of this input buffer.
 * @param y8hist Buffer storing the histogram for the Y (luma) channel.
 * @param hist_size Total number of buckets in the histogram, all channels included.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rgb24frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t *y8hist,
				size_t hist_size, uint16_t nval);

/**
 * @brief Collect an histogram for the Y channel, obtained from the values of an RGGB8 frame.
 *
 * @param buf Buffer of pixels in bayer format (1 byte per pixel) to collect the statistics from..
 * @param buf_size Size of this input buffer.
 * @param width Width of the lines in number of pixels.
 * @param y8hist Buffer storing the histogram for the Y (luma) channel.
 * @param hist_size Total number of buckets in the histogram, all channels included.
 * @param nval The number of values to collect in order to perform the statistics.
 */
void pixel_rggb8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for the Y channel, obtained from the values of an GBRG8 frame.
 * @copydetails pixel_rggb8frame_to_y8hist()
 */
void pixel_gbrg8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for the Y channel, obtained from the values of an BGGR8 frame.
 * @copydetails pixel_rggb8frame_to_y8hist()
 */
void pixel_bggr8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval);
/**
 * @brief Collect an histogram for the Y channel, obtained from the values of an GRBG8 frame.
 * @copydetails pixel_rggb8frame_to_y8hist()
 */
void pixel_grbg8frame_to_y8hist(const uint8_t *buf, size_t buf_size, uint16_t width,
				uint16_t *y8hist, size_t hist_size, uint16_t nval);

#endif /* ZEPHYR_INCLUDE_PIXEL_STATS_H */
