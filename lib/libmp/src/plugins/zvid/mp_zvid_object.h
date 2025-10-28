/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid object.
 */

#ifndef __MP_ZVID_OBJECT_H__
#define __MP_ZVID_OBJECT_H__

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/video.h>

#include <src/core/mp_caps.h>
#include <src/core/mp_element.h>
#include <src/core/mp_query.h>

#include "mp_zvid_buffer_pool.h"

#define MP_ZVID_OBJECT(self) ((struct mp_zvid_object *)self)

/**
 * @brief Video Object structure
 *
 * This structure represents a video object that provides common video
 * functionalities for the video source and transform elements.
 */
struct mp_zvid_object {
	/** Pointer to the video device */
	const struct device *vdev;
	/** Video buffer type */
	enum video_buf_type type;
	/** Associated buffer pool */
	struct mp_zvid_buffer_pool pool;
	/** Rectangle area to crop the input image in pixels */
	struct video_rect crop;
};

/**
 * @brief Convert a @ref mp_structure to a @ref video_format_cap
 *
 * Populates a @ref video_format_cap with information from a @ref mp_structure.
 *
 * @param structure Pointer to @ref mp_structure containing video format information
 * @param vfc Pointer to video_format_cap structure to be populated
 *
 * @return 0 on success or a negative errno code on failure
 */
int mp_structure_to_vfc(struct mp_structure *structure, struct video_format_cap *vfc);

/**
 * @brief Set a property on the video object
 *
 * @param zvid_obj Pointer to the @ref struct mp_zvid_object
 * @param key Property key/control ID
 * @param val Property value to set
 * @param pad_caps set
 *
 * @return 0 on success or a negative errno code on failure
 */
int mp_zvid_object_set_property(struct mp_zvid_object *zvid_obj, uint32_t key, const void *val,
				struct mp_caps **pad_caps);

/**
 * @brief Get a property from the video object
 *
 * @param zvid_obj Pointer to the @ref struct mp_zvid_object
 * @param key Property key/control ID to retrieve
 * @param val Pointer to store the retrieved property value
 *
 * @return 0 on success or a negative errno code on failure
 */
int mp_zvid_object_get_property(struct mp_zvid_object *zvid_obj, uint32_t key, void *val);

/**
 * @brief Get capabilities of the video object
 *
 * Queries the underlying video device for its capabilities including pixel formats,
 * resolutions, and frame rates.
 *
 * @param zvid_obj Pointer to the @ref struct mp_zvid_object
 *
 * @return Pointer to @ref struct mp_caps containing device capabilities, or NULL on error
 */
struct mp_caps *mp_zvid_object_get_caps(struct mp_zvid_object *zvid_obj);

/**
 * @brief Set capabilities on the video object
 *
 * Configures the video device with the specified capabilities (video format
 * and frame rate). The capabilities must be fixed (not ranges).
 *
 * @param zvid_obj Pointer to the @ref struct mp_zvid_object
 * @param caps Pointer to @ref struct mp_caps with fixed capabilities to set
 *
 * @return true on success and false on failure
 */
bool mp_zvid_object_set_caps(struct mp_zvid_object *zvid_obj, struct mp_caps *caps);

/**
 * @brief Decide buffer allocation parameters
 *
 * Decide buffer allocation parameters between the video object's buffer pool
 * and the query requirements. The video object always uses its own buffer
 * pool and just negotiates the configuration parameters.
 *
 * @param zvid_obj Pointer to the @ref struct mp_zvid_object
 * @param query Pointer to @ref struct mp_query containing allocation requirements
 *
 * @return true on success and false on failure
 */
bool mp_zvid_object_decide_allocation(struct mp_zvid_object *zvid_obj, struct mp_query *query);

#endif /* __MP_ZVID_OBJECT_H__ */
