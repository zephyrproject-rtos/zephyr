/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_SW_PIPELINE_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_SW_PIPELINE_H_

#include <zephyr/drivers/video.h>
#include <zephyr/pixel/stream.h>

/**
 * @brief Set the source device that feeds data into the pipeline.
 *
 * This submits a video device to use as source, required to make API
 * calls such as @ref video_enum_frmival() work.
 *
 * @param dev The video-sw-pipeline device that is fed from a source device.
 * @param source_dev The video device that feeds the data.
 */
void video_sw_pipeline_set_source(const struct device *dev, const struct device *source_dev);

/**
 * @brief Configure the zephyr,video-sw-pipeline device.
 *
 * The input format will be applied to the device preceding it as configured in the devicetree.
 * The input and output formats must match those of the pixel pipeline.
 *
 * @param dev The video-sw-pipeline device that runs this pipeline.
 * @param strm Pixel stream structs chained together as produced by @ref pixel_stream().
 * @param pixfmt_in Input video pixel format.
 * @param width_in Input width.
 * @param height_in Input height.
 * @param pixfmt_out Output video pixel format.
 * @param width_out Output width.
 * @param height_out Output height.
 */
void video_sw_pipeline_set_pipeline(const struct device *dev, struct pixel_stream *strm,
				    uint32_t pixfmt_in, uint16_t width_in, uint16_t height_in,
				    uint32_t pixfmt_out, uint16_t width_out, uint16_t height_out);

/**
 * @brief Set the loader function that pushes data from the video frame to the device.
 *
 * The pixel library performs operation on typically small intermediate buffers of just one or a
 * few lines, but some video operations require the full frame or most of it. The first element
 * of the pipeline is a special case as it has the entire input frame available.
 *
 * This setter submit such a loader function that performs this initial frame-to-stream operation.
 *
 * @param dev The video-sw-pipeline device that runs this loader function.
 * @param fn The function that has a full frame available and loads data into the stream
 *           line-by-line.
 */
void video_sw_pipeline_set_loader(const struct device *dev,
				  void (*fn)(struct pixel_stream *s, const uint8_t *b, size_t n));

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_SW_PIPELINE_H_ */
