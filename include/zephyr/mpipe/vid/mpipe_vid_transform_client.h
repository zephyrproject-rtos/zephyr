/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Client-side video transform element for multi-core pipelines.
 * @ingroup mpipe_vid_transform_clients
 *
 * Extends the generic transform client with video-specific RPC callbacks
 * to offload video processing to a remote core.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_H_

/**
 * @defgroup mpipe_vid_transform_clients Transform Clients
 * @ingroup mpipe_vid
 * @brief Client-side video transform elements.
 * @{
 */

#include <zephyr/drivers/video.h>

#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_transform_client.h>

#include <zephyr/mpipe/vid/mpipe_vid_buffer_pool_client.h>

struct mpipe_element;

/**
 * @brief Client-side video transform element structure.
 *
 * Extends @ref mpipe_transform_client with video buffer pools and RPC
 * function pointers for capability query, format negotiation, and
 * cap transformation across cores.
 */
struct mpipe_vid_transform_client {
	/** Base transform client structure */
	struct mpipe_transform_client transform;
	/** Input buffer pool */
	struct mpipe_vid_buffer_pool_client in_pool;
	/** Output buffer pool */
	struct mpipe_vid_buffer_pool_client out_pool;
	/**
	 * @brief RPC callback to query buffer pool capabilities.
	 *
	 * @param direction    Pad direction (see @ref mpipe_pad_direction).
	 * @param min_buffers  Output: minimum number of buffers required.
	 * @param buf_align    Output: required buffer alignment.
	 *
	 * @return 0 on success or a negative errno code on failure.
	 */
	int32_t (*get_buf_caps_rpc)(enum mpipe_pad_direction direction, uint8_t *min_buffers,
				    uint16_t *buf_align);
	/**
	 * @brief RPC callback to enumerate video format capabilities.
	 *
	 * @param direction Pad direction (see @ref mpipe_pad_direction).
	 * @param ind       Index to iterate through available capabilities.
	 * @param vfc       Output: video format capability at @p ind.
	 *
	 * @return 0 on success or a negative errno code on failure.
	 */
	int32_t (*get_format_caps_rpc)(enum mpipe_pad_direction direction, uint8_t ind,
				       struct video_format_cap *vfc);
	/**
	 * @brief RPC callback to set the video format.
	 *
	 * @param fmt Video format to apply; the server may update size and pitch.
	 *
	 * @return 0 on success or a negative errno code on failure.
	 */
	int32_t (*set_format_rpc)(struct video_format *fmt);
	/**
	 * @brief RPC callback to transform a capability to the opposite pad.
	 *
	 * @param direction Pad direction to transform from (see @ref mpipe_pad_direction).
	 * @param ind       Index to iterate through transformed capabilities.
	 * @param vfc       Input video format capability.
	 * @param other_vfc Output: transformed video format capability.
	 *
	 * @return 0 on success or a negative errno code on failure.
	 */
	int32_t (*transform_cap_rpc)(enum mpipe_pad_direction direction, uint16_t ind,
				     const struct video_format_cap *vfc,
				     struct video_format_cap *other_vfc);
};

/**
 * @brief Initialize a client-side video transform element.
 *
 * @param vtc Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_vid_transform_client_init(struct mpipe_vid_transform_client *vtc, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_TRANSFORM_CLIENT_H_ */
