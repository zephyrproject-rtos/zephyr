/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pipeline: the top-level bin that runs a graph.
 * @ingroup mpipe_pipeline
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_PIPELINE_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_PIPELINE_H_

/**
 * @defgroup mpipe_pipeline Pipelines
 * @ingroup mpipe_framework
 * @brief The top-level bin, and what actually runs a graph.
 *
 * A pipeline is the outermost @ref mpipe_bin, which gives it three jobs no
 * inner bin has. It owns the thread that acquires buffers from the source and
 * pushes each one downstream until a sink consumes it; a queue element gives a
 * part of the graph a thread of its own. It orders the teardown: buffers still
 * in flight are dropped before the children dismantle their pools, and the
 * thread is joined only once the children have drained. A pause is not a
 * teardown, so whatever is queued survives and a resume continues without
 * loss. And it folds the end of the stream: a graph with several sinks
 * produces one end-of-stream message per sink, and only the last one reaches
 * the application.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_thread.h>

/**
 * @brief Properties for a pipeline
 *
 * Enumeration of properties that can be configured for a pipeline
 */
enum mpipe_prop_pipeline {
	/**
	 * Thread scheduling priority used when the pipeline thread is created.
	 * Defaults to @kconfig{CONFIG_MPIPE_THREAD_DEFAULT_PRIORITY}.
	 */
	MPIPE_PROP_PIPELINE_THREAD_PRIORITY,
};

/**
 * @brief A pipeline: the top-level bin that runs a graph.
 *
 * Adds to @ref mpipe_bin the thread that drives the source and the
 * end-of-stream accounting that lets a graph with several sinks report
 * completion exactly once.
 */
struct mpipe {
	/** Base bin container */
	struct mpipe_bin bin;
	/** Thread associated with the pipeline */
	struct mpipe_thread thread;
	/** Number of sink elements in the pipeline (computed on READY->PAUSED) */
	uint32_t num_sinks;
	/** Number of EOS messages seen so far during the current run */
	atomic_t eos_count;
};

/**
 * @brief Initialize a pipeline
 *
 * Initializes the pipeline structure, including the base bin and its message channel.
 *
 * @param pipe Pointer to the @ref mpipe to initialize.
 * @param id   Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_pipeline_init(struct mpipe *pipe, uint8_t id);

/**
 * @brief Push a buffer downstream starting from a given source pad
 *
 * Walks downstream from an element's @p src_pad, calling each next element's chain_fn
 * until a sink is reached, a chain_fn fails, or the output buffer is NULL.
 *
 * The chain function owns the buffer it is given and releases it whether it
 * succeeds or fails, so the walk does not release it on a chain error. The walk
 * does release the buffer itself in the two cases where no chain function is
 * reached: the source pad has no peer, and the peer pad is flushing.
 *
 * @param src_pad Source pad to start pushing from (its peer's chain_fn is first called)
 * @param buffer Buffer to push (ownership transferred)
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_push_buffer(struct mpipe_pad *src_pad, struct net_buf *buffer);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_PIPELINE_H_ */
