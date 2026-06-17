/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Video object shared by zvid source and transform elements.
 *
 * Provides common video device interaction (capabilities, format negotiation,
 * buffer pool management) used by @ref mp_zvid_src and @ref mp_zvid_transform.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_OBJECT_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/video.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/zvid/mp_zvid_buffer_pool.h>

struct mp_dispatch;

/**
 * @brief Video object structure.
 *
 * Wraps a Zephyr video device with its associated buffer pool and
 * crop region, providing helpers for capability and format negotiation.
 */
struct mp_zvid_object {
	/** Pointer to the video device */
	const struct device *vdev;
	/** Video buffer type (input or output) */
	enum video_buf_type type;
	/** Associated buffer pool */
	struct mp_zvid_buffer_pool pool;
	/** Rectangle area to crop the input image in pixels */
	struct video_rect crop;
};

/**
 * @brief Convert an @ref mp_structure to a @ref video_format_cap.
 *
 * @param structure Pointer to the @ref mp_structure containing video format information.
 * @param vfc       Pointer to @ref video_format_cap to populate.
 *
 * @return 0 on success or a negative errno code on failure.
 */
int mp_structure_to_vfc(struct mp_structure *structure, struct video_format_cap *vfc);

/**
 * @brief Set a property on the video object.
 *
 * @param zvid_obj Pointer to the @ref mp_zvid_object.
 * @param key      Property key / control ID.
 * @param val      Property value to set.
 *
 * @return 0 on success or a negative errno code on failure.
 */
int mp_zvid_object_set_property(struct mp_zvid_object *zvid_obj, uint32_t key, const void *val);

/**
 * @brief Get a property from the video object.
 *
 * @param zvid_obj Pointer to the @ref mp_zvid_object.
 * @param key      Property key / control ID to retrieve.
 * @param val      Pointer to store the retrieved property value.
 *
 * @return 0 on success or a negative errno code on failure.
 */
int mp_zvid_object_get_property(struct mp_zvid_object *zvid_obj, uint32_t key, void *val);

/**
 * @brief Get capabilities of the video object.
 *
 * Queries the underlying video device for its supported pixel formats,
 * resolutions, and frame rates.
 *
 * @param zvid_obj Pointer to the @ref mp_zvid_object.
 *
 * @return Pointer to an @ref mp_caps on success, or NULL on error.
 */
struct mp_caps *mp_zvid_object_get_caps(struct mp_zvid_object *zvid_obj);

/**
 * @brief Set capabilities on the video object.
 *
 * Configures the video device with the specified fixed capabilities
 * (video format and frame rate).
 *
 * @param zvid_obj Pointer to the @ref mp_zvid_object.
 * @param caps     Pointer to @ref mp_caps with fixed capabilities to set.
 *
 * @return 0 on success or a negative errno code on failure.
 */
int mp_zvid_object_set_caps(struct mp_zvid_object *zvid_obj, struct mp_caps *caps);

/**
 * @brief Decide buffer allocation parameters.
 *
 * Negotiates buffer allocation between the video object's own pool and
 * the requirements carried by @p query. The video object always uses
 * its own pool and only negotiates configuration parameters.
 *
 * @param zvid_obj Pointer to the @ref mp_zvid_object.
 * @param query    Pointer to @ref mp_dispatch containing allocation requirements.
 *
 * @return 0 on success or a negative errno code on failure.
 */
int mp_zvid_object_decide_allocation(struct mp_zvid_object *zvid_obj, struct mp_dispatch *query);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_OBJECT_H_ */
