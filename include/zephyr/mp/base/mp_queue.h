/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Queue element for pipeline-level threading.
 *
 * The queue element decouples a pipeline into two segments running on two different threads.
 * Upstream deposits buffers into the queue's internal buffer queue; then a dedicated
 * downstream thread pulls buffers from that queue and push to the rest of the pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MP_BASE_MP_BASE_QUEUE_H_
#define ZEPHYR_INCLUDE_MP_BASE_MP_BASE_QUEUE_H_

/**
 * @defgroup mp_queue Queues
 * @ingroup mp_base
 * @brief Pipeline-level threading element
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_thread.h>
#include <zephyr/mp/mp_transform.h>

/**
 * @brief Queue Property Identifiers
 */
enum {
	/** Nmber of buffers the queue can hold */
	MP_PROP_BASE_QUEUE_SIZE = MP_PROP_TRANSFORM_LAST,
};

/**
 * @brief Queue Element Structure
 *
 * The queue element acts as a thread boundary in a pipeline. Its chainfn enqueues
 * buffers into an internal buffer queue. A dedicated thread then dequeues buffers
 * and drives downstream elements.
 */
struct mp_queue {
	/** Base transform element */
	struct mp_transform transform;
	/** Dedicated thread for downstream processing */
	struct mp_thread thread;
	/** Message queue for storing incoming buffer pointers */
	struct k_msgq msgq;
	/** Backing storage for the message queue, its size equals to queue's max size + 2
	 * (for eos and pause sentinels)
	 */
	char msgq_buffer[(CONFIG_MP_BASE_QUEUE_MAX_SIZE + 2) * sizeof(void *)];
	/** Number of buffers the queue can hold bounded by CONFIG_MP_BASE_QUEUE_MAX_SIZE */
	uint8_t size;
	/**
	 * Flushing flag. When set (on PAUSED -> READY), the chainfn drops incoming
	 * buffers instead of enqueueing them. This releases any upstream producer
	 * blocked in k_msgq_put() during teardown and prevents a late buffer from
	 * leaking into an already-drained queue (e.g. behind a tee).
	 */
	atomic_t flushing;
};

/**
 * @brief Initialize a queue element
 *
 * @param self Pointer to the @ref mp_element to initialize as a queue
 */
void mp_queue_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_BASE_MP_BASE_QUEUE_H_ */
