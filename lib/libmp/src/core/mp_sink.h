/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpSink.
 */

#ifndef __MP_SINK_H__
#define __MP_SINK_H__

#include "mp_buffer.h"
#include "mp_caps.h"
#include "mp_element.h"
#include "mp_element_factory.h"
#include "mp_pad.h"
#include "mp_query.h"

#define MP_SINK(self) ((MpSink *)self)

typedef struct _MpSink MpSink;

/**
 * @brief Sink Element Structure
 *
 * Represents a sink element in the media pipeline. Sink elements are terminal
 * elements that consume data from upstream elements through their sink pad.
 */
struct _MpSink {
	/** Base element structure */
	MpElement element;
	/** Input pad for receiving data */
	MpPad sinkpad;
	/** Buffer pool */
	MpBufferPool *pool;
	/**
	 * @brief Set capabilities callback
	 * @param sink Pointer to the sink element
	 * @param caps Capabilities to set
	 * @return true if capabilities were set successfully, false otherwise
	 */
	bool (*set_caps)(MpSink *sink, MpCaps *caps);
	/**
	 * @brief Get capabilities callback
	 * @param sink Pointer to the sink element
	 * @return Pointer to @ref MpCaps or NULL if not available
	 */
	MpCaps *(*get_caps)(MpSink *sink);
	/**
	 * @brief Propose allocation callback
	 * @param self Pointer to the sink element
	 * @param query Allocation query to process
	 * @return true if allocation proposal was successful, false otherwise
	 */
	bool (*propose_allocation)(MpSink *self, MpQuery *query);
};

/**
 * @brief Initialize a sink element
 *
 * Initializes the base sink element structure, sets up the sink pad,
 * and configures default callbacks for query and event handling.
 *
 * @param self Pointer to the @ref MpElement to initialize as a sink
 */
void mp_sink_init(MpElement *self);

/**
 * @brief Set property on sink element
 *
 * @param obj Pointer to the @ref MpObject
 * @param key Property key identifier
 * @param val Pointer to the property value
 * @return 0 on success, negative error code on failure
 */
int mp_sink_set_property(MpObject *obj, uint32_t key, const void *val);

/**
 * @brief Get property from sink element
 *
 * @param obj Pointer to the @ref MpObject
 * @param key Property key identifier
 * @param val Pointer to store the retrieved property value
 * @return 0 on success, negative error code on failure
 */
int mp_sink_get_property(MpObject *obj, uint32_t key, void *val);

#endif /* __MP_SINK_H__ */
