/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pad: the point where two elements meet.
 * @ingroup mpipe_pad
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_PAD_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_PAD_H_

/**
 * @defgroup mpipe_pad Pad
 * @ingroup mpipe_framework
 * @brief The point where two elements meet.
 *
 * A pad is an element's input or output connector. It has a direction - a
 * source pad (MPIPE_PAD_SRC) emits data, a sink pad (MPIPE_PAD_SINK) receives
 * it - and @ref mpipe_pad_link joins one of each. The link is recorded on both
 * sides through their @c peer pointers, and that pair of pointers is the path
 * every buffer, event and query travels.
 *
 * A pad also holds the capability it settled on, one @ref mpipe_structure by
 * value. Until a negotiation has run it constrains nothing, and
 * @ref mpipe_pad_set_caps with NULL puts it back that way, which is what the
 * PAUSED to READY transition does.
 *
 * @section mpipe_pad_traffic Buffers, events and queries
 *
 * Three kinds of traffic cross a link, each with its own hook:
 *
 * - a **buffer** arrives at a sink pad's @c process_fn. The processing function owns
 *   the buffer it is given and must release it even when it fails; the caller
 *   never touches it again either way.
 * - an **event** announces something and is sent with
 *   @ref mpipe_pad_send_event - a format to apply, or the end of the stream.
 *   The default handler forwards it across the element to the peers on the
 *   other side.
 * - a **query** asks something and is sent with @ref mpipe_pad_query. The
 *   answer is written into storage the asker owns, so a query allocates
 *   nothing.
 *
 * Events and queries are both carried by @ref mpipe_dispatch; what decides
 * whether a dispatch asks or announces is the function it is passed to, not
 * anything in the dispatch itself.
 *
 * @section mpipe_pad_enum Enumerating capabilities
 *
 * An element that supports several formats does not describe them as one
 * capability holding a list - there is no list type, because there is no
 * allocation. It answers @c enum_caps_fn once per index, producing one
 * capability at a time, and negotiation walks those indices. See
 * @ref mpipe_pad_enum_caps for the three-way return that drives the walk.
 *
 * @section mpipe_pad_presence Presence
 *
 * A pad records a presence describing when it is meant to exist -
 * MPIPE_PAD_ALWAYS as soon as the element is built, MPIPE_PAD_SOMETIMES coming
 * and going with the stream, MPIPE_PAD_REQUEST only once asked for. Only
 * MPIPE_PAD_ALWAYS is implemented today: the field is stored and nothing acts
 * on it, so treat the other two as reserved.
 *
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_object.h>

struct mpipe_dispatch;

/**
 * @brief The direction of a pad
 */
enum mpipe_pad_direction {
	/** Direction is unknown */
	MPIPE_PAD_UNKNOWN,
	/** The pad is a source pad */
	MPIPE_PAD_SRC,
	/** The pad is a sink pad */
	MPIPE_PAD_SINK
};

/**
 * @brief The operating mode of a @ref mpipe_pad
 *
 * Defines if the pad operates in push or pull mode or none of them.
 */
enum mpipe_pad_mode {
	/** Pad will not handle dataflow */
	MPIPE_PAD_MODE_NONE,
	/** Pad handles dataflow in push mode */
	MPIPE_PAD_MODE_PUSH,
	/** Pad handles dataflow in pull mode */
	MPIPE_PAD_MODE_PULL
};

/**
 * @brief The presence of a pad
 */
enum mpipe_pad_presence {
	/** The pad is always present */
	MPIPE_PAD_ALWAYS,
	/** The pad will be present depending on the media stream */
	MPIPE_PAD_SOMETIMES,
	/** The pad is only available on request */
	MPIPE_PAD_REQUEST
};

/**
 * @brief Pad structure
 *
 * The pad structure represents a connection point of an element.
 * Pads are used to negotiate capabilities and transfer data between elements.
 */
struct mpipe_pad {
	/** Base object */
	struct mpipe_object object;
	/** Pad direction, cannot change after creating the pad */
	enum mpipe_pad_direction direction;
	/** Pad presence */
	enum mpipe_pad_presence presence;
	/** Operating mode */
	enum mpipe_pad_mode mode;
	/** Pointer to the peer pad this pad is linked to */
	struct mpipe_pad *peer;
	/** Pad's capability. ANY until one is negotiated, reset back to ANY on PAUSED to READY */
	struct mpipe_structure caps;
	/** Flushing gate. While set, buffers are dropped instead of processed */
	atomic_t flushing;

	/**
	 * @brief Process a buffer
	 *
	 * The element's processing function, called on its sink pad for every
	 * buffer pushed to it. It owns @p in_buf and releases it whether it
	 * succeeds or fails, and hands back what it produced: a new buffer from
	 * its pool, the same buffer transformed in place or passed through, or
	 * NULL when it consumed the input, which ends the push.
	 *
	 * @param pad Sink pad the buffer arrives on
	 * @param in_buf Buffer to process
	 * @param[out] out_buf Buffer produced, or NULL when the input was consumed
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*process_fn)(struct mpipe_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf);
	/**
	 * @brief Answer a query
	 *
	 * @param pad Pad the query arrives on
	 * @param query Query to answer in place
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*query_fn)(struct mpipe_pad *pad, struct mpipe_dispatch *query);
	/**
	 * @brief Handle an event
	 *
	 * @param pad Pad the event arrives on
	 * @param event Event to act on and forward
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*event_fn)(struct mpipe_pad *pad, struct mpipe_dispatch *event);
	/**
	 * @brief Produce one of the pad's supported capabilities
	 *
	 * This is how an element says what it supports. The framework calls it
	 * with @p index 0, 1, 2 and so on, one capability per call, until it
	 * reports -ENOENT, so several formats live on the index rather than in
	 * a list. A capability known at build time is copied out of a static
	 * structure; one only known at run time is built by querying the
	 * driver. When @p filter is given, produce the capability narrowed by
	 * it, which @ref mpipe_pad_enum_filter does, and report -EAGAIN when
	 * this index cannot satisfy it, so the walk moves on to the next one.
	 * The default produces the pad's own capability at index 0 and nothing
	 * past it.
	 *
	 * @param pad Pad to enumerate
	 * @param index Zero-based index of the capability
	 * @param filter Capability to narrow by, may be NULL
	 * @param[out] out Caller storage for the capability
	 *
	 * @retval 0 Success.
	 * @retval -EAGAIN This index cannot satisfy @p filter
	 * @retval -ENOENT Past the last capability
	 */
	int (*enum_caps_fn)(struct mpipe_pad *pad, uint32_t index,
			    const struct mpipe_structure *filter, struct mpipe_structure *out);
};

/**
 * @brief Produce one of the pad's supported caps.
 *
 * Calls the pad's enum_caps_fn, and stops asking past UINT16_MAX so an
 * implementation that never reports the end fails instead of hanging.
 *
 * @param pad Pad to enumerate.
 * @param index Zero-based index of the capability.
 * @param filter Optional structure to narrow the capability by, may be NULL.
 * @param[out] out Caller storage for the capability.
 *
 * @retval 0 Success.
 * @retval -EAGAIN This index cannot satisfy @p filter
 * @retval -ENOENT Past the last capability
 * @retval -EINVAL The pad has no enum_caps_fn
 */
int mpipe_pad_enum_caps(struct mpipe_pad *pad, uint32_t index, const struct mpipe_structure *filter,
			struct mpipe_structure *out);

/**
 * @brief Produce the pad's first supported capability that a filter accepts.
 *
 * @param pad Pad to enumerate.
 * @param filter Capability to narrow by, may be NULL or ANY.
 * @param[out] out Caller storage for the capability.
 *
 * @retval 0 Success.
 * @retval -ENODATA No capability is accepted
 * @return Any other negative errno the pad's enum_caps_fn returns
 */
int mpipe_pad_enum_first(struct mpipe_pad *pad, const struct mpipe_structure *filter,
			 struct mpipe_structure *out);

/**
 * @brief Answer a caps query with the first capability its filter accepts.
 *
 * Enumerates the pad and writes the capability the negotiation would settle on
 * back into @p query. This is what an element with nothing to transform, a
 * source or a sink, installs as its query handler.
 *
 * @param pad Pad to enumerate.
 * @param query Caps query to answer, carrying the filter on entry.
 *
 * @return 0 on success, negative errno on other failures
 * @retval -ENODATA The pad has no capability the filter accepts
 */
int mpipe_pad_answer_caps_query(struct mpipe_pad *pad, struct mpipe_dispatch *query);

/**
 * @brief Narrow one enumerated capability by an enumeration filter.
 *
 * The epilogue every @ref mpipe_pad enum_caps_fn shares: hand back @p candidate
 * when there is no filter, otherwise narrow it and report that this index
 * cannot satisfy the filter so the caller moves on to the next one.
 *
 * @param candidate Pointer to the capability this index produced.
 * @param filter Capability to narrow by, may be NULL.
 * @param[out] out Pointer to storage for the result.
 *
 * @retval 0 Success.
 * @retval -EAGAIN @p candidate cannot satisfy @p filter
 */
int mpipe_pad_enum_filter(const struct mpipe_structure *candidate,
			  const struct mpipe_structure *filter, struct mpipe_structure *out);

/**
 * @brief Initialize a pad
 *
 * Initializes an existing @ref mpipe_pad structure with the specified parameters.
 *
 * @param pad Pointer to the @ref mpipe_pad to initialize
 * @param id Unique ID of the pad instance in the element
 * @param direction Direction of the pad (@ref mpipe_pad_direction)
 * @param presence Presence of the pad (@ref mpipe_pad_presence)
 */
void mpipe_pad_init(struct mpipe_pad *pad, uint8_t id, enum mpipe_pad_direction direction,
		    enum mpipe_pad_presence presence);

/**
 * @brief Set the pad's capability.
 *
 * Copies @p caps into the pad. Passing NULL resets the pad to constraining
 * nothing, which is what a re-negotiation starts from.
 *
 * @param pad Pad to set the capability on.
 * @param caps Capability to copy in, or NULL to reset to ANY.
 *
 * @retval 0 Success.
 */
int mpipe_pad_set_caps(struct mpipe_pad *pad, const struct mpipe_structure *caps);

/**
 * @brief Link two pads together
 *
 * Links a source pad to a sink pad, establishing a connection for data flow.
 * Both pads will have their peer pointers set to each other. Neither pad may
 * be NULL; that is an assertion, not an error.
 *
 * @param src_pad Source pad to link
 * @param sink_pad Sink pad to link
 */
void mpipe_pad_link(struct mpipe_pad *src_pad, struct mpipe_pad *sink_pad);

/**
 * @brief Send an event to a pad
 *
 * Sends an event to the specified pad using the pad's event function.
 *
 * @param pad Pointer to the @ref mpipe_pad where the event should be sent
 * @param event Pointer to the @ref mpipe_dispatch to send
 *
 * @retval 0 Success.
 * @retval -ENOTSUP The pad has no event function
 * @return Any negative errno the pad's event function returns
 */
int mpipe_pad_send_event(struct mpipe_pad *pad, struct mpipe_dispatch *event);

/**
 * @brief Default event handler for pads
 *
 * Forwards an event received on @p pad to the peers of all opposite-side pads
 * in the same element. If @p pad is a sink pad, the event is forwarded to the
 * peer of each source pad; if @p pad is a source pad, it is forwarded to the
 * peer of each sink pad.
 *
 * If the element has only source pads or only sink pads, there are no
 * opposite-side pads to send the event to, so the function returns
 * -ENOTSUP.
 *
 * @param pad Pointer to the @ref mpipe_pad that received the event
 * @param event Pointer to the @ref mpipe_dispatch to send
 *
 * @retval 0 The event reached at least one peer.
 * @retval -ENOTSUP No opposite-side pad has a peer, or every peer refused the
 *         event. A peer's own error is logged and not propagated.
 */
int mpipe_pad_send_event_default(struct mpipe_pad *pad, struct mpipe_dispatch *event);

/**
 * @brief Send a query to a pad
 *
 * Sends a query to the pad using the pad's query function.
 *
 * A caps query is answered into the storage it carries, so one that carries
 * none is refused before the pad's query function is called. That is what lets
 * a query function dereference @ref mpipe_dispatch::caps without checking it.
 *
 * @param pad Pointer to the @ref mpipe_pad to send query to, must not be NULL
 * @param query Pointer to the @ref mpipe_dispatch to send, must not be NULL.
 *              A caps query must carry the capability storage to answer into.
 *
 * @retval 0 Success.
 * @retval -EINVAL A caps query carries no capability storage to answer into
 * @retval -ENOTSUP The pad has no query function
 * @retval -ENODATA A caps query was answered with an empty capability
 * @return Any negative errno the pad's query function returns
 */
int mpipe_pad_query(struct mpipe_pad *pad, struct mpipe_dispatch *query);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_PAD_H_ */
