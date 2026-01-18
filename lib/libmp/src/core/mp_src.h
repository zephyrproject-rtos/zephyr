/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_src.
 */

#ifndef __MP_SRC_H__
#define __MP_SRC_H__

#include "mp_buffer.h"
#include "mp_element.h"
#include "mp_pad.h"

#define MP_SRC(self) ((struct mp_src *)self)

/**
 * @brief Base Source Element Structure
 *
 * The source element is responsible for generating data and pushing it downstream.
 * It contains a source pad for output.
 */
struct mp_src {
	/** Base element structure */
	struct mp_element element;
	/** Source pad for output data */
	struct mp_pad srcpad;
	/** Number of buffers that the source outputs before sending EOS */
	uint32_t num_buffers;
	/** Buffer pool for managing output buffers */
	struct mp_buffer_pool *pool;
	/** Callback to set capabilities on the source pad */
	bool (*set_caps)(struct mp_src *src, struct mp_caps *caps);
	/** Callback to get capabilities from the source pad */
	struct mp_caps *(*get_caps)(struct mp_src *src);
	/** Callback to decide buffer allocation strategy */
	bool (*decide_allocation)(struct mp_src *self, struct mp_query *query);
};

/**
 * @brief Initialize a source element
 *
 * This function initializes the base source element structure, including
 * the source pad and default callbacks.
 *
 * @param self Pointer to the @ref struct mp_element to initialize as a source
 */
void mp_src_init(struct mp_element *self);

/**
 * @brief Set property on source element
 *
 * @param obj Pointer to the @ref struct mp_object (source element)
 * @param key Property key identifier
 * @param val Pointer to the property value to set
 * @return 0 on success, negative error code on failure
 */
int mp_src_set_property(struct mp_object *obj, uint32_t key, const void *val);

/**
 * @brief Get property from source element
 *
 * @param obj Pointer to the @ref struct mp_object (source element)
 * @param key Property key identifier
 * @param val Pointer to store the retrieved property value
 * @return 0 on success, negative error code on failure
 */
int mp_src_get_property(struct mp_object *obj, uint32_t key, void *val);

#endif /* __MP_SRC_H__ */
