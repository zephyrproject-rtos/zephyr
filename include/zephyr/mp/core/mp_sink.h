/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_sink.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_SINK_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_SINK_H_

/**
 * @defgroup mp_sink Sinks
 * @ingroup mp_core
 * @brief Terminal elements that consume data from a pipeline.
 *
 * @{
 */

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

struct mp_dispatch;

/**
 * @brief Sink Element Structure
 *
 * Represents a sink element in the media pipeline. Sink elements are terminal
 * elements that consume data from upstream elements through their sink pad.
 */
struct mp_sink {
	/** Base element structure */
	struct mp_element element;
	/** Input pad for receiving data */
	struct mp_pad sinkpad;
	/** @cond INTERNAL_HIDDEN */
	/** Pointer to the supported caps */
	struct mp_caps *sink_caps;
	/** @endcond */
	/** Buffer pool */
	struct mp_buffer_pool *pool;
	/**
	 * @brief Get the supported caps of the element.
	 *
	 * To get the current caps, use sinkpad->caps instead
	 *
	 * @param sink Pointer to the sink element
	 * @return Pointer to @ref mp_caps or NULL if not available
	 */
	struct mp_caps *(*get_caps)(struct mp_sink *sink);
	/**
	 * @brief Set a given caps to the element
	 * @param sink Pointer to the sink element
	 * @param caps Capabilities to set
	 * @return 0 on success, negative errno on failure
	 */
	int (*set_caps)(struct mp_sink *sink, struct mp_caps *caps);
	/**
	 * @brief Propose allocation strategy to the upstream peer
	 * @param self Pointer to the sink element
	 * @param query Allocation query to process
	 * @return 0 on success, negative errno on failure
	 */
	int (*propose_allocation)(struct mp_sink *self, struct mp_dispatch *query);
};

/**
 * @brief Initialize a sink element
 *
 * Initializes the base sink element structure, sets up the sink pad,
 * and configures default callbacks for query and event handling.
 *
 * @param self Pointer to the @ref mp_element to initialize as a sink
 */
void mp_sink_init(struct mp_element *self);

/**
 * @brief Update the capabilities of a sink element
 *
 * Updates the sink element's capabilities with the provided caps structure.
 * This function is typically called when the capabilities of the sink need
 * to be modified or reconfigured during runtime.
 *
 * @param sink Pointer to the sink element
 * @param caps Pointer to the new capabilities to apply
 */
void mp_sink_update_caps(struct mp_sink *sink, struct mp_caps *caps);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_SINK_H_ */
