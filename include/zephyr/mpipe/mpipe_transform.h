/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Transform: an element that turns input buffers into output buffers.
 * @ingroup mpipe_transform
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_H_

/**
 * @defgroup mpipe_transform Transforms
 * @ingroup mpipe_framework
 * @brief Elements with an input and an output, and the middle of a negotiation.
 *
 * A transform has one sink pad and one source pad: a buffer arrives on one side
 * and something leaves on the other. What "something" means is the element's
 * choice, and @ref mpipe_transform_mode names the three shapes - produce a new
 * buffer from an output pool, rewrite the incoming buffer where it lies, or
 * pass it through untouched.
 *
 * A transform is the interesting case in both negotiations, because it is the
 * only base that has to reconcile two sides.
 *
 * @section mpipe_transform_caps Crossing a capability
 *
 * @c transform_caps is what maps a capability on one side to what the other
 * side could then be. A decoder taking one compressed format may be able to
 * emit several raw ones, so this is a one-to-many relation - and, since nothing
 * may allocate, those alternatives live on an index rather than in a returned
 * set: one transformation per index, walked until the element reports there are
 * no more.
 *
 * Answering a capability query therefore nests two walks: for each capability
 * this side supports, for each transformation of it, ask the peer on the other
 * side. The first combination the peer accepts wins, and the peer's answer is
 * kept on the far pad because the event pass needs it to narrow back down.
 *
 * When the format event later arrives, the transform crosses it the same way
 * and **forwards it before applying it to its own pads**. The order matters:
 * an element is allowed to rewire the graph from its set_caps, so applying
 * first would change the path the event still has to travel.
 *
 * @section mpipe_transform_pool Buffer pools
 *
 * On a buffer pool query a transform forwards downstream first and decides
 * afterwards, so it always sees its downstream's proposal before choosing; then
 * it answers upstream with a proposal of its own. See @c propose_buffer_pool
 * and @c decide_buffer_pool below for what each side of that owes.
 *
 * @{
 */

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @brief Properties for a transform element
 *
 * Enumeration of properties that can be configured for base transform
 */
enum mpipe_prop_transform {
	/** Last transform property marker (for validation/iteration) */
	MPIPE_PROP_TRANSFORM_LAST,
};

/**
 * @brief Operating modes of a transform element
 *
 * The mode specifies how input and output buffers are handled.
 */
enum mpipe_transform_mode {
	/** The buffer is kept intact. */
	MPIPE_MODE_PASSTHROUGH = 0,
	/** The input buffer is directly modified. Input and output buffers are the same. */
	MPIPE_MODE_INPLACE = 1,
	/** The output buffer is allocated and differs from the input buffer. */
	MPIPE_MODE_NORMAL = 2,
};

/**
 * @brief Transform element structure
 *
 * Base structure for all transform elements in the media pipeline.
 * Transform elements process data from their sink pad and output
 * the result to their source pad.
 */
struct mpipe_transform {
	/** Base element structure */
	struct mpipe_element element;
	/** Sink pad for receiving input data */
	struct mpipe_pad sink_pad;
	/** Source pad for sending output data */
	struct mpipe_pad src_pad;
	/** Pointer to the input buffer pool for allocating input buffers */
	struct mpipe_buffer_pool *in_pool;
	/** Pointer to the output buffer pool for allocating output buffers */
	struct mpipe_buffer_pool *out_pool;
	/** Operating mode determining buffer handling strategy */
	enum mpipe_transform_mode mode;

	/**
	 * @brief Set a given caps to an element's pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref mpipe_pad_direction)
	 * @param caps Capability to set (@ref mpipe_structure)
	 * @return 0 on success, negative errno on failure
	 */
	int (*set_caps)(struct mpipe_transform *transform, enum mpipe_pad_direction direction,
			const struct mpipe_structure *caps);
	/**
	 * @brief Produce one transformation of a capability across the element
	 *
	 * An element commonly maps a single input capability to several output
	 * ones: a decoder that can emit any of N pixel formats, a converter with
	 * N reachable formats. Those alternatives live on @p index rather than in
	 * a returned set, so a negotiation walks them one at a time and the fan
	 * out is never materialized.
	 *
	 * @param self Pointer to the transform element
	 * @param direction Direction of the pad to transform the capability to
	 *                  (@ref mpipe_pad_direction)
	 * @param in Capability on the opposite pad to transform
	 * @param index Index of the transformation to produce, starting at 0
	 * @param out Caller storage receiving the transformation
	 *
	 * @return 0 when @p out holds a transformation, -EAGAIN when this index
	 *         produces none but a later one may, -ENOENT past the last one,
	 *         or another negative errno on failure
	 */
	int (*transform_caps)(struct mpipe_transform *self, enum mpipe_pad_direction direction,
			      const struct mpipe_structure *in, uint32_t index,
			      struct mpipe_structure *out);

	/**
	 * @brief Propose a buffer pool to upstream
	 *
	 * The transform element may propose either its entire input buffer pool
	 * (set the query's pool pointer; the pool's own config is the proposal)
	 * or a bare config (write the query's pool_cfg). A proposed pool's ops
	 * are then intended to be called by the upstream element. Before
	 * proposing a pool, rebuild its config baseline from your source of
	 * truth: the live config is the previous negotiation's applied result,
	 * and proposing it as-is makes peer demands accumulate across replays.
	 *
	 * For in-place transform, the same pool may be used for both input and output. If the
	 * pool is proposed to upstream, the element has to use @ref mpipe_transform::in_pool to
	 * point to the pool and leave @ref mpipe_transform::out_pool as NULL so that the pool is
	 * configured / started only by the upstream and not by the transform element itself.
	 *
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref mpipe_dispatch)
	 * @return 0 on success, negative errno on failure
	 */
	int (*propose_buffer_pool)(struct mpipe_transform *self, struct mpipe_dispatch *query);
	/**
	 * @brief Decide the buffer pool for downstream
	 *
	 * The downstream proposal is either an entire pool (the query's pool
	 * pointer; its config is the proposal) or a bare config (the query's
	 * pool_cfg). Apply a demand to a proposed pool only through
	 * @ref mpipe_buffer_pool_set_config, which the pool may refuse - keep a
	 * fallback. An element deciding for its own pool rebuilds its config
	 * baseline here on every negotiation before absorbing the query's
	 * demands, so nothing accumulates across replays.
	 *
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref mpipe_dispatch)
	 * @return 0 on success, negative errno on failure
	 */
	int (*decide_buffer_pool)(struct mpipe_transform *self, struct mpipe_dispatch *query);
};

/**
 * @brief Initialize a transform element
 *
 * This function initializes the base transform element structure, sets up sink
 * and source pads, and configures default function pointers for element
 * operations. A concrete transform calls it first from its own init, with the
 * same id, and then overrides what it needs.
 *
 * @param transform Pointer to the @ref mpipe_transform to initialize.
 * @param id        Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_transform_init(struct mpipe_transform *transform, uint8_t id);

/**
 * @brief Set capabilities on a transform element's pad.
 *
 * @param transform Pointer to the transform element.
 * @param direction Direction of the pad to set caps on (@ref mpipe_pad_direction).
 * @param caps Pointer to the capabilities to set.
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_transform_set_caps(struct mpipe_transform *transform, enum mpipe_pad_direction direction,
			     const struct mpipe_structure *caps);

/**
 * @brief Change state function for the base transform element
 *
 * On the PAUSED to READY transition this resets the negotiated pad caps back to
 * ANY so a subsequent re-negotiation starts fresh.
 * Derived transforms that override change_state must chain to this base
 * function to inherit that behavior.
 *
 * @param self Pointer to the @ref mpipe_element
 * @param transition Transition state, see @ref mpipe_state_change
 *
 * @return One of @ref mpipe_state_change_return
 */
enum mpipe_state_change_return mpipe_transform_change_state(struct mpipe_element *self,
							    enum mpipe_state_change transition);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_TRANSFORM_H_ */
