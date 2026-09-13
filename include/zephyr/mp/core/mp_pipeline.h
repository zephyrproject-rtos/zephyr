/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_PIPELINE_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_PIPELINE_H_

/**
 * @defgroup mp_pipeline Pipelines
 * @ingroup mp_core
 * @brief Top-level pipeline container and runtime control.
 * @{
 */

#include <stdint.h>

#include <zephyr/net_buf.h>

#include <zephyr/mp/core/mp_bin.h>
#include <zephyr/mp/core/mp_thread.h>

/**
 * @{
 */

/**
 * @brief struct mp_pipeline structure
 *
 * Contains all the state and timing information needed to manage
 * a complete media processing pipeline.
 */
struct mp_pipeline {
	/** Base bin container */
	struct mp_bin bin;
	/** Thread associated with the pipeline */
	struct mp_thread thread;
	/** The running time - total time spent in PLAYING state without being flushed */
	uint64_t stream_time;
	/**
	 * Extra delay added to base_time to compensate for computing delays when setting
	 * elements to PLAYING
	 */
	uint64_t delay;
};

/**
 * @brief Initialize a pipeline
 *
 * Initializes the pipeline structure, including the base bin and message bus.
 *
 * @param self Pointer to the @ref mp_element to initialize as a pipeline
 */
void mp_pipeline_init(struct mp_element *self);

/**
 * @brief Push a buffer downstream starting from a given source pad
 *
 * Walks downstream from an element's @p srcpad, calling each next element's chainfn
 * until a sink is reached, a chainfn fails, or output buffer is NULL
 *
 * On chainfn error the buffer is unreffed internally.
 *
 * @param srcpad Source pad to start pushing from (its peer's chainfn is first called)
 * @param buffer Buffer to push (ownership transferred)
 *
 * @return 0 on success, negative errno on failure
 */
int mp_pipeline_push_buffer(struct mp_pad *srcpad, struct net_buf *buffer);

/**
 * @}
 */

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_PIPELINE_H_ */
