/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_transform.h>

LOG_MODULE_REGISTER(mpipe_transform, CONFIG_MPIPE_LOG_LEVEL);

#define MPIPE_PAD_SINK_ID 0
#define MPIPE_PAD_SRC_ID  1

static int mpipe_transform_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				    struct net_buf **out_buf)
{
	ARG_UNUSED(pad);

	/* Default implementation for MPIPE_MODE_PASSTHROUGH - return same buffer */
	*out_buf = in_buf;

	return 0;
}

int mpipe_transform_set_caps(struct mpipe_transform *transform, enum mpipe_pad_direction direction,
			     const struct mpipe_structure *caps)
{
	__ASSERT_NO_MSG(transform != NULL);

	if (direction == MPIPE_PAD_SINK) {
		return mpipe_pad_set_caps(&transform->sink_pad, caps);
	}

	if (direction == MPIPE_PAD_SRC) {
		return mpipe_pad_set_caps(&transform->src_pad, caps);
	}

	return -EINVAL;
}

static int mpipe_transform_transform_caps(struct mpipe_transform *self,
					  enum mpipe_pad_direction direction,
					  const struct mpipe_structure *in, uint32_t index,
					  struct mpipe_structure *out)
{
	ARG_UNUSED(self);
	ARG_UNUSED(direction);

	/* Passthrough by default: the capability crosses the element unchanged */
	if (index > 0U) {
		return -ENOENT;
	}

	*out = *in;

	return 0;
}

/*
 * Produce the transformation at or after *index, skipping the indices that
 * yield nothing, and report through *index the one it settled on so the caller
 * can resume past it.
 */
static int mpipe_transform_enum_caps(struct mpipe_transform *self,
				     enum mpipe_pad_direction direction,
				     const struct mpipe_structure *in, uint32_t *index,
				     struct mpipe_structure *out)
{
	int ret;

	for (; *index <= UINT16_MAX; (*index)++) {
		ret = self->transform_caps(self, direction, in, *index, out);
		if (ret != -EAGAIN) {
			return ret;
		}
	}

	/* The search ends on -ENOENT, so one that is never reported would run forever */
	LOG_ERR("element id = %u: transform_caps never reported the end of its transformations",
		self->element.object.id);

	return -ENOENT;
}

/*
 * Transform the peer's answer back to this pad's side and narrow it by the
 * candidate the attempt started from. The answer may map back to several
 * capabilities, so they are walked and the first that still contains the
 * candidate wins.
 */
static int mpipe_transform_narrow_to_candidate(struct mpipe_transform *self,
					       struct mpipe_pad *this_pad,
					       const struct mpipe_structure *answer,
					       const struct mpipe_structure *candidate,
					       struct mpipe_structure *out)
{
	struct mpipe_structure back;
	int ret;

	for (uint32_t index = 0;; index++) {
		ret = mpipe_transform_enum_caps(self, this_pad->direction, answer, &index, &back);
		if (ret != 0) {
			return -ENODATA;
		}

		/* Narrow by the candidate this attempt started from */
		ret = mpipe_structure_intersect(&back, candidate, out);
		if (ret == 0) {
			return 0;
		}
	}
}

/*
 * Offer one transformation of a candidate to the peer on the other side and
 * keep what comes back when the peer accepts it. Reports -ENODATA when the peer
 * refuses it or its answer does not lead back to the candidate, which is the
 * caller's cue to offer the next transformation.
 */
static int mpipe_transform_offer(struct mpipe_transform *self, struct mpipe_pad *this_pad,
				 struct mpipe_pad *other_pad,
				 const struct mpipe_structure *candidate,
				 const struct mpipe_structure *transformed,
				 struct mpipe_dispatch *query, struct mpipe_structure *out)
{
	int ret;

	/* Query the peer pad with the transformed caps */
	*query->caps = *transformed;

	ret = mpipe_pad_query(other_pad->peer, query);
	if (ret < 0) {
		/*
		 * Capabilities are offered one at a time, so a peer refusing one
		 * of them is an ordinary step of the negotiation and not a
		 * failure. The source reports the error if none is accepted.
		 */
		LOG_DBG("element id = %u: peer refused the transformed caps",
			self->element.object.id);
		return -ENODATA;
	}

	if (mpipe_structure_is_empty(query->caps)) {
		return -ENODATA;
	}

	ret = mpipe_transform_narrow_to_candidate(self, this_pad, query->caps, candidate, out);
	if (ret != 0) {
		return ret;
	}

	/*
	 * Keep the peer's answer at other_pad: the caps event needs it to narrow
	 * the transformations of the fixated capability back down. Published only
	 * now that the attempt has succeeded, so a refused candidate leaves the
	 * pad as it found it.
	 */
	return mpipe_pad_set_caps(other_pad, query->caps);
}

/*
 * Offer one of this pad's capabilities across the element. The element may map
 * it to several capabilities on the other side, so those are offered to the
 * peer one at a time as well. Reports -ENODATA when this candidate leads
 * nowhere, which is the caller's cue to offer the next one.
 */
static int mpipe_transform_try_candidate(struct mpipe_transform *self, struct mpipe_pad *this_pad,
					 struct mpipe_pad *other_pad,
					 const struct mpipe_structure *candidate,
					 struct mpipe_dispatch *query, struct mpipe_structure *out)
{
	struct mpipe_structure transformed;
	int ret;

	for (uint32_t index = 0;; index++) {
		ret = mpipe_transform_enum_caps(self, other_pad->direction, candidate, &index,
						&transformed);
		if (ret == -ENOENT) {
			return -ENODATA;
		}

		if (ret != 0) {
			return ret;
		}

		ret = mpipe_transform_offer(self, this_pad, other_pad, candidate, &transformed,
					    query, out);
		if (ret != -ENODATA) {
			return ret;
		}
	}
}

static inline int mpipe_transform_query_caps(struct mpipe_transform *self,
					     enum mpipe_pad_direction direction,
					     struct mpipe_dispatch *query)
{
	struct mpipe_pad *this_pad, *other_pad;
	struct mpipe_structure candidate;
	struct mpipe_structure result;
	struct mpipe_structure filter;
	int ret;

	switch (direction) {
	case MPIPE_PAD_SINK:
		this_pad = &self->sink_pad;
		other_pad = &self->src_pad;
		break;
	case MPIPE_PAD_SRC:
		this_pad = &self->src_pad;
		other_pad = &self->sink_pad;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * Keep a copy of the incoming filter: offering a candidate to the peer
	 * reuses the query and overwrites its caps.
	 */
	filter = *query->caps;

	for (uint32_t index = 0;; index++) {
		ret = mpipe_pad_enum_caps(this_pad, index, &filter, &candidate);
		if (ret == -EAGAIN) {
			continue;
		}

		if (ret == -ENOENT) {
			/*
			 * Same distinction as mpipe_src_negotiate(): an element with no capability
			 * at all cannot answer any query, which is a caller error.
			 */
			ret = (index == 0) ? -EINVAL : -ENODATA;
			break;
		}

		if (ret != 0) {
			break;
		}

		ret = mpipe_transform_try_candidate(self, this_pad, other_pad, &candidate, query,
						    &result);
		if (ret != -ENODATA) {
			break;
		}
	}

	if (ret != 0) {
		return ret;
	}

	/* Answer the upstream query, writing into the asker's storage */
	*query->caps = result;

	return 0;
}

static int mpipe_transform_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	struct mpipe_transform *self = (struct mpipe_transform *)pad->object.container;
	int ret;

	switch (query->type) {
	case MPIPE_DISPATCH_CAPS:
		return mpipe_transform_query_caps(self, pad->direction, query);
	case MPIPE_DISPATCH_BUFFER_POOL:
		struct mpipe_dispatch peer_query = {
			.type = MPIPE_DISPATCH_BUFFER_POOL,
			.caps = &self->src_pad.caps,
		};

		/* Query the downstream */
		ret = mpipe_pad_query(self->src_pad.peer, &peer_query);
		if (ret < 0) {
			return ret;
		}

		/* Decide the buffer pool for downstream */
		if (self->decide_buffer_pool != NULL) {
			ret = self->decide_buffer_pool(self, &peer_query);
			if (ret < 0) {
				return ret;
			}
		}

		/* Configure/start the output buffer pool */
		if (self->mode == MPIPE_MODE_NORMAL) {
			ret = mpipe_buffer_pool_configure(self->out_pool, &self->src_pad.caps);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to configure output transform buffer pool");
				return ret;
			}

			ret = mpipe_buffer_pool_start(self->out_pool);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to start output transform buffer pool");
				return ret;
			}
		}

		/* Propose the buffer pool to upstream */
		if (self->propose_buffer_pool != NULL) {
			return self->propose_buffer_pool(self, query);
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

/*
 * Cross a fixed capability over to the other pad at caps event time. A
 * transformation unfixes it again, so every transformation is narrowed by the
 * peer's answer kept at other_pad during the caps query and the first that
 * survives is fixated.
 */
static int mpipe_transform_cross_caps(struct mpipe_transform *self, struct mpipe_pad *other_pad,
				      const struct mpipe_structure *in, struct mpipe_structure *out)
{
	struct mpipe_structure transformed;
	struct mpipe_structure narrowed;
	int ret;

	for (uint32_t index = 0;; index++) {
		ret = mpipe_transform_enum_caps(self, other_pad->direction, in, &index,
						&transformed);
		if (ret == -ENOENT) {
			return -ENODATA;
		}

		if (ret != 0) {
			return ret;
		}

		ret = mpipe_structure_intersect(&transformed, &other_pad->caps, &narrowed);
		if (ret != 0) {
			continue;
		}

		ret = mpipe_structure_fixate(&narrowed, out);
		if (ret == 0) {
			return 0;
		}
	}
}

static int mpipe_transform_event(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	int ret;

	switch (event->type) {
	case MPIPE_DISPATCH_EOS:
		LOG_DBG("MPIPE_DISPATCH_EOS");
		return mpipe_pad_send_event_default(pad, event);
	case MPIPE_DISPATCH_CAPS:
		LOG_DBG("MPIPE_DISPATCH_CAPS");
		struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
		struct mpipe_pad *other_pad;
		struct mpipe_structure incoming;
		struct mpipe_structure fixated;

		other_pad = (pad->direction == MPIPE_PAD_SINK) ? &transform->src_pad
							       : &transform->sink_pad;

		/*
		 * A caps event carries a fixed format. One carrying none, or one
		 * that constrains nothing, means the source could not fixate,
		 * and there is nothing to cross over.
		 */
		if (event->caps == NULL || mpipe_structure_is_any(event->caps)) {
			return -EINVAL;
		}

		/*
		 * Take a copy: the event's own capability is replaced below with
		 * what crosses to the other side, but this side still has to be
		 * applied afterwards.
		 */
		incoming = *event->caps;

		ret = mpipe_transform_cross_caps(transform, other_pad, &incoming, &fixated);
		if (ret != 0) {
			return ret;
		}

		*event->caps = fixated;
		ret = mpipe_pad_send_event(other_pad->peer, event);

		/*
		 * Apply this side only once the peer has accepted, and only then
		 * the other side: a caps_filter drops itself out of the graph from
		 * set_caps(), so the event must already have been forwarded.
		 */
		if (ret == 0) {
			ret = transform->set_caps(transform, pad->direction, &incoming);
		}

		if (ret == 0) {
			ret = transform->set_caps(transform, other_pad->direction, &fixated);
		}

		return ret;
	default:
		return -ENOTSUP;
	}
}

enum mpipe_state_change_return mpipe_transform_change_state(struct mpipe_element *self,
							    enum mpipe_state_change transition)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)self;

	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_element_reset_pad_caps(self);

		/*
		 * The transform started the pool it draws from - its own or an
		 * adopted proposal - so it stops it here; otherwise the pool
		 * stays started across runs and can never be reconfigured.
		 */
		if (transform->out_pool != NULL) {
			(void)mpipe_buffer_pool_stop(transform->out_pool);
		}
		break;
	default:
		break;
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

int mpipe_transform_init(struct mpipe_transform *transform, uint8_t id)
{
	__ASSERT_NO_MSG(transform != NULL);

	struct mpipe_element *self = &transform->element;
	int ret = mpipe_element_init(self, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "transform");

	mpipe_pad_init(&transform->sink_pad, MPIPE_PAD_SINK_ID, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);
	mpipe_pad_init(&transform->src_pad, MPIPE_PAD_SRC_ID, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &transform->sink_pad);
	mpipe_element_add_pad(self, &transform->src_pad);

	self->change_state = mpipe_transform_change_state;

	transform->mode = MPIPE_MODE_PASSTHROUGH;
	transform->set_caps = mpipe_transform_set_caps;
	transform->transform_caps = mpipe_transform_transform_caps;
	transform->sink_pad.chain_fn = mpipe_transform_chain_fn;
	transform->sink_pad.query_fn = mpipe_transform_query;
	transform->src_pad.query_fn = mpipe_transform_query;
	transform->sink_pad.event_fn = mpipe_transform_event;
	transform->src_pad.event_fn = mpipe_transform_event;
	transform->decide_buffer_pool = NULL;
	transform->propose_buffer_pool = NULL;

	return 0;
}
