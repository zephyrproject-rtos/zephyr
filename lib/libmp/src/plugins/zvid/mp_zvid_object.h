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

#define MP_ZVIDOBJECT(self) ((MpZvidObject *)self)

/**
 * @brief Video Object structure
 *
 * This structure represents a video object that provides common video
 * functionalities for the video source and transform elements.
 */
typedef struct _MpZvidObject {
	/** Pointer to the video device */
	const struct device *vdev;
	/** Video buffer type */
	enum video_buf_type type;
	/** Associated buffer pool */
	MpZvidBufferPool pool;
} MpZvidObject;

/**
 * @brief Set a property on the video object
 *
 * @param zvid_obj Pointer to the @ref MpZvidObject
 * @param key Property key/control ID
 * @param val Property value to set
 *
 * @return 0 on success or a negative errno codes on failure
 */
int mp_zvid_object_set_property(MpZvidObject *zvid_obj, uint32_t key, const void *val);

/**
 * @brief Get a property from the video object
 *
 * @param zvid_obj Pointer to the @ref MpZvidObject
 * @param key Property key/control ID to retrieve
 * @param val Pointer to store the retrieved property value
 *
 * @return 0 on success or a negative errno codes on failure
 */
int mp_zvid_object_get_property(MpZvidObject *zvid_obj, uint32_t key, void *val);

/**
 * @brief Get capabilities of the video object
 *
 * Queries the underlying video device for its capabilities including pixel formats,
 * resolutions, and frame rates.
 *
 * @param zvid_obj Pointer to the @ref MpZvidObject
 *
 * @return Pointer to @ref MpCaps containing device capabilities, or NULL on error
 */
MpCaps *mp_zvid_object_get_caps(MpZvidObject *zvid_obj);

/**
 * @brief Set capabilities on the video object
 *
 * Configures the video device with the specified capabilities (video format
 * and frame rate). The capabilities must be fixed (not ranges).
 *
 * @param zvid_obj Pointer to the @ref MpZvidObject
 * @param caps Pointer to @ref MpCaps with fixed capabilities to set
 *
 * @return true on success and false on failure
 */
bool mp_zvid_object_set_caps(MpZvidObject *zvid_obj, MpCaps *caps);

/**
 * @brief Decide buffer allocation parameters
 *
 * Decide buffer allocation parameters between the video object's buffer pool
 * and the query requirements. The video object always uses its own buffer
 * pool and just negotiates the configuration parameters.
 *
 * @param zvid_obj Pointer to the @ref MpZvidObject
 * @param query Pointer to @ref MpQuery containing allocation requirements
 *
 * @return true on success and false on failure
 */
bool mp_zvid_object_decide_allocation(MpZvidObject *zvid_obj, MpQuery *query);

#endif /* __MP_ZVID_OBJECT_H__ */
