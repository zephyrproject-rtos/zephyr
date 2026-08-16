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
 * A pipeline is the outermost @ref mpipe_bin. Being the outermost is what gives
 * it three jobs no inner bin has.
 *
 * It **owns the thread**. One thread sits at the head of the graph acquiring
 * buffers from the source and pushing each one downstream through the chain
 * functions until a sink consumes it. An element that needs its own thread -
 * to decouple two halves of a graph - gets one by putting a queue between them.
 *
 * It **orders the teardown**. Going down from PAUSED to READY, the pipeline
 * raises a flushing gate on every pad before the children dismantle their pools,
 * so a buffer still in flight is dropped rather than pushed into an element that
 * has already been torn down; and it joins the thread only after the children
 * have drained, because a child still holding the thread in a full queue would
 * otherwise deadlock the join. Going from PLAYING to PAUSED it does neither -
 * a pause is not a teardown, so whatever is queued survives and a resume
 * continues without loss.
 *
 * It **folds the end of the stream**. A graph with several sinks produces one
 * end-of-stream message per sink; the pipeline counts them and passes on only
 * the last, so the application is told once and never tears a graph down while
 * a branch is still running.
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
