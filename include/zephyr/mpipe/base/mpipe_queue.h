/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Queue element for pipeline-level threading.
 * @ingroup mpipe_queue
 *
 * The queue element decouples a pipeline into two segments running on two different threads.
 * Upstream deposits buffers into the queue's internal buffer queue; then a dedicated
 * downstream thread pulls buffers from that queue and push to the rest of the pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_QUEUE_H_
#define ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_QUEUE_H_

/**
 * @defgroup mpipe_queue Queues
 * @ingroup mpipe_base
 * @brief Pipeline-level threading element.
 * @{
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_thread.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Queue Property Identifiers
 */
enum {
	/** Number of buffers the queue can hold */
	MPIPE_PROP_BASE_QUEUE_SIZE = MPIPE_PROP_TRANSFORM_LAST,
	/** Scheduling priority of the queue's thread */
	MPIPE_PROP_BASE_QUEUE_THREAD_PRIORITY,
	/** What a full queue does with a new buffer, an @ref mpipe_base_queue_leak */
	MPIPE_PROP_BASE_QUEUE_LEAK,
};

/**
 * @brief What a full queue does with a new buffer
 *
 * A queue is first a thread boundary that loses nothing, so by default a
 * full queue holds the producer until the consumer frees a slot. A live
 * source feeding a slower branch would rather drop: a leak policy bounds
 * the latency instead of the producer. End of stream is never dropped.
 */
enum mpipe_base_queue_leak {
	/** Block the producer until a slot is free */
	MPIPE_BASE_QUEUE_LEAK_NONE = 0,
	/** Drop the oldest queued buffer: the consumer always gets the freshest */
	MPIPE_BASE_QUEUE_LEAK_OLDEST,
	/** Drop the arriving buffer: the queued ones are delivered in order */
	MPIPE_BASE_QUEUE_LEAK_NEWEST,
};

/**
 * @brief Queue Element Structure
 *
 * The queue element acts as a thread boundary in a pipeline. Its chain_fn enqueues
 * buffers into an internal buffer queue. A dedicated thread then dequeues buffers
 * and drives downstream elements.
 */
struct mpipe_queue {
	/** Base transform element */
	struct mpipe_transform transform;
	/** Dedicated thread for downstream processing */
	struct mpipe_thread thread;
	/**
	 * Queue for storing incoming buffer pointers.
	 *
	 * A k_fifo would not do here as it keeps no storage of its own: it chains its
	 * items through a link field inside each item, so a given buffer can be on one
	 * fifo at a time. A tee pushes the same buffer to every branch, and a branch
	 * may start with a queue, so one buffer must be able to wait in several queues
	 * at once. The k_msgq stores its own copy of the pointer which makes that possible.
	 */
	struct k_msgq msgq;
	/**
	 * Backing storage for the message queue, sized for the largest queue
	 * plus the EOS and pause sentinels
	 */
	char msgq_buffer[(CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE + 2) * sizeof(void *)];
	/**
	 * Number of buffers the queue can hold, bounded by
	 * CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE and applied on READY -> PAUSED
	 */
	uint8_t size;
	/** Leak policy of a full queue, an @ref mpipe_base_queue_leak */
	uint8_t leak;
	/**
	 * Flushing flag. When set (on PAUSED -> READY), the chain_fn drops incoming
	 * buffers instead of enqueuing them. This releases any upstream producer
	 * blocked in k_msgq_put() during teardown and prevents a late buffer from
	 * leaking into an already-drained queue (e.g. behind a tee).
	 */
	atomic_t flushing;
};

/**
 * @brief Initialize a queue element
 *
 * @param queue Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_queue_init(struct mpipe_queue *queue, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_BASE_MPIPE_QUEUE_H_ */
