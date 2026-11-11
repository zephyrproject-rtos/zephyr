/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief Heimann HTPA sensor frame formats and layout definitions.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_SENSOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_SENSOR_H_

#include <zephyr/drivers/video.h>

/**
 * @defgroup video_htpa HTPA sensor definitions
 * @ingroup video_interface
 * @{
 */

/** Frame width of the HTPA 120x84 sensor, in pixels. */
#define VIDEO_HTPA_120X84_WIDTH 120U
/** Frame height of the HTPA 120x84 sensor, in pixels. */
#define VIDEO_HTPA_120X84_HEIGHT 84U
/** Number of pixel blocks in each half of an HTPA 120x84 frame. */
#define VIDEO_HTPA_120X84_BLOCK_COUNT 6U
/** Length of an HTPA 120x84 block, in bytes. */
#define VIDEO_HTPA_120X84_BLOCK_LENGTH 1686U
/** Offset of the first sample in an HTPA 120x84 block, in bytes. */
#define VIDEO_HTPA_120X84_DATA_OFFSET 6U
/** Number of samples in an HTPA 120x84 block. */
#define VIDEO_HTPA_120X84_SAMPLES_PER_BLOCK 840U

/** Frame width of the HTPA 160x120 sensor, in pixels. */
#define VIDEO_HTPA_160X120_WIDTH 160U
/** Frame height of the HTPA 160x120 sensor, in pixels. */
#define VIDEO_HTPA_160X120_HEIGHT 120U
/** Number of pixel blocks in each half of an HTPA 160x120 frame. */
#define VIDEO_HTPA_160X120_BLOCK_COUNT 12U
/** Length of an HTPA 160x120 block, in bytes. */
#define VIDEO_HTPA_160X120_BLOCK_LENGTH 1606U
/** Offset of the first sample in an HTPA 160x120 block, in bytes. */
#define VIDEO_HTPA_160X120_DATA_OFFSET 6U
/** Number of samples in an HTPA 160x120 block. */
#define VIDEO_HTPA_160X120_SAMPLES_PER_BLOCK 800U

/**
 * Raw sensor blocks, each including its sensor header: top electrical offsets,
 * bottom electrical offsets, all top pixel blocks, then all bottom pixel blocks.
 * Pixel and offset samples are big-endian unsigned 16-bit integers. Each half's
 * pixel blocks contain rows in order. The bottom half starts at the last image
 * row. The offsets are reused for each pixel block in the corresponding half.
 * Block dimensions depend on the sensor resolution.
 */
#define VIDEO_PIX_FMT_HTPA_RAW VIDEO_FOURCC('H', 'T', 'P', 'A')

/** Layout of a VIDEO_PIX_FMT_HTPA_RAW frame. */
struct video_htpa_raw_layout {
	/** Number of pixel blocks in each half of the frame. */
	uint32_t block_count;
	/** Length of each block, including its header, in bytes. */
	uint32_t block_length;
	/** Offset of the first sample within each block, in bytes. */
	uint32_t data_offset;
	/** Number of pixel or offset samples in each block. */
	uint32_t samples_per_block;
};

/** Initializer for the HTPA 120x84 raw frame layout. */
#define VIDEO_HTPA_120X84_RAW_LAYOUT \
	{ \
		.block_count = VIDEO_HTPA_120X84_BLOCK_COUNT, \
		.block_length = VIDEO_HTPA_120X84_BLOCK_LENGTH, \
		.data_offset = VIDEO_HTPA_120X84_DATA_OFFSET, \
		.samples_per_block = VIDEO_HTPA_120X84_SAMPLES_PER_BLOCK, \
	}

/** Initializer for the HTPA 160x120 raw frame layout. */
#define VIDEO_HTPA_160X120_RAW_LAYOUT \
	{ \
		.block_count = VIDEO_HTPA_160X120_BLOCK_COUNT, \
		.block_length = VIDEO_HTPA_160X120_BLOCK_LENGTH, \
		.data_offset = VIDEO_HTPA_160X120_DATA_OFFSET, \
		.samples_per_block = VIDEO_HTPA_160X120_SAMPLES_PER_BLOCK, \
	}

/** Signed native-endian 16-bit corrected samples, saturated to 16 bits, in row-major order. */
#define VIDEO_PIX_FMT_HTPA_S16 VIDEO_FOURCC('H', 'T', 'S', '1')

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_SENSOR_H_ */
