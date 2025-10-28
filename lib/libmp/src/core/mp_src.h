/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpSrc.
 */

#ifndef __MP_SRC_H__
#define __MP_SRC_H__

#include "mp_buffer.h"
#include "mp_element.h"
#include "mp_element_factory.h"
#include "mp_pad.h"

#define MP_SRC(self) ((MpSrc *)self)

typedef struct _MpSrc MpSrc;

/**
 * @brief Base Source Element Structure
 *
 * The source element is responsible for generating data and pushing it downstream.
 * It contains a source pad for output.
 */
struct _MpSrc {
	/** Base element structure */
	MpElement element;
	/** Source pad for output data */
	MpPad srcpad;
	/** Number of buffers to allocate */
	uint8_t num_buffers;
	/** Buffer pool for managing output buffers */
	MpBufferPool *pool;
	/** Callback to set capabilities on the source pad */
	bool (*set_caps)(MpSrc *src, MpCaps *caps);
	/** Callback to get capabilities from the source pad */
	MpCaps *(*get_caps)(MpSrc *src);
	/** Callback to decide buffer allocation strategy */
	bool (*decide_allocation)(MpSrc *self, MpQuery *query);
};

/**
 * @brief Initialize a source element
 *
 * This function initializes the base source element structure, including
 * the source pad and default callbacks.
 *
 * @param self Pointer to the @ref MpElement to initialize as a source
 */
void mp_src_init(MpElement *self);

/**
 * @brief Set property on source element
 *
 * @param obj Pointer to the @ref MpObject (source element)
 * @param key Property key identifier
 * @param val Pointer to the property value to set
 * @return 0 on success, negative error code on failure
 */
int mp_src_set_property(MpObject *obj, uint32_t key, const void *val);

/**
 * @brief Get property from source element
 *
 * @param obj Pointer to the @ref MpObject (source element)
 * @param key Property key identifier
 * @param val Pointer to store the retrieved property value
 * @return 0 on success, negative error code on failure
 */
int mp_src_get_property(MpObject *obj, uint32_t key, void *val);

#endif /* __MP_SRC_H__ */
