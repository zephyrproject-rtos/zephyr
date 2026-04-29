/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid transform client element.
 */

#ifndef __MP_ZVID_TRANSFORM_CLIENT_H__
#define __MP_ZVID_TRANSFORM_CLIENT_H__

#include <zephyr/drivers/video.h>

#include <src/core/mp_pad.h>
#include <src/core/mp_transform_client.h>

#include "mp_zvid_buffer_pool_client.h"

struct mp_element;

#define MP_ZVID_TRANSFORM_CLIENT(self) ((struct mp_zvid_transform_client *)self)

/**
 * @brief Transform client element structure
 *
 * Base structure for all transform elements on the client side of multi-core pipeline.
 * These elements call RPC to the server side to off-load the processing work.
 */
struct mp_zvid_transform_client {
	/** Base transform client structure */
	struct mp_transform_client transform;
	/** Input buffer pool*/
	struct mp_zvid_buffer_pool_client inpool;
	/** Output buffer pool*/
	struct mp_zvid_buffer_pool_client outpool;
	/**
	 * @brief RPC function to get buffer pool capabilities
	 * @param direction Pad direction (@ref enum mp_pad_direction)
	 * @param min_buffers Minimum number of buffers required for the pool
	 * @param buf_align Alignment of buffers in the pool
	 * @return 0 on success, errno on failure
	 */
	int32_t (*get_buf_caps_rpc)(enum mp_pad_direction direction, uint8_t *min_buffers,
				 uint16_t *buf_align);
	/**
	 * @brief RPC function to get video format capabilities
	 * @param direction Pad direction (@ref enum mp_pad_direction)
	 * @param ind Index to iterate through available format capabilities
	 * @param vfc Output video format caps (@ref struct video_format_cap)
	 * @return 0 on success, errno on failure
	 */
	int32_t (*get_format_caps_rpc)(enum mp_pad_direction direction, uint8_t ind,
				       struct video_format_cap *vfc);
	/**
	 * @brief RPC function to set video format
	 * @param fmt The video format to set (@ref struct video_format). Format size and pitch may
	 * be returned by the server.
	 * @return 0 on success, errno on failure
	 */
	int32_t (*set_format_rpc)(struct video_format *fmt);
	/**
	 * @brief RPC function to transform video format caps from one end to the other end
	 * @param direction Pad direction to transform (@ref enum mp_pad_direction)
	 * @param ind Index to iterate through available video format capabilities
	 * @param vfc Input video format caps to be transformed (@ref struct video_format_cap)
	 * @param other_vfc Output transformed video format caps (@ref struct video_format_cap)
	 * @return 0 on success, errno on failure
	 */
	int32_t (*transform_cap_rpc)(enum mp_pad_direction direction, uint16_t ind,
				     const struct video_format_cap *vfc,
				     struct video_format_cap *other_vfc);
};

/**
 * @brief Initialize a video transform element on the client side
 *
 * @param self Pointer to the @ref struct mp_element to initialize as a video transform
 */
void mp_zvid_transform_client_init(struct mp_element *self);

#endif /* __MP_ZVID_TRANSFORM_CLIENT_H__ */
