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

#define SINK_PAD_ID 0
#define SRC_PAD_ID  1

static int mpipe_transform_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				      struct net_buf **out_buf)
{
	ARG_UNUSED(pad);

	/* Default implementation for MPIPE_TRANSFORM_MODE_PASSTHROUGH - return same buffer */
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

/* Map the peer's answer back to this side and narrow it by the candidate */
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

/* Offer one transformation to the peer; -ENODATA says try the next one */
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

	/* Keep the peer's answer at other_pad: the caps event narrows against it */
	return mpipe_pad_set_caps(other_pad, query->caps);
}

/* Offer one candidate through each of its transformations; -ENODATA says try the next */
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

	/* Offering a candidate overwrites the query's caps, so keep the filter */
	filter = *query->caps;

	for (uint32_t index = 0;; index++) {
		ret = mpipe_pad_enum_caps(this_pad, index, &filter, &candidate);
		if (ret == -EAGAIN) {
			continue;
		}

		if (ret == -ENOENT) {
			/* No capability at all is a caller error, not a failed negotiation */
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
		if (self->mode == MPIPE_TRANSFORM_MODE_NORMAL) {
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

/* Cross a fixed capability to the other pad, narrowed by the answer kept there */
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

		/* A caps event carries a fixed format; none means the source could not fixate */
		if (event->caps == NULL || mpipe_structure_is_any(event->caps)) {
			return -EINVAL;
		}

		/* Copy: the event's capability is replaced by what crosses over */
		incoming = *event->caps;

		ret = mpipe_transform_cross_caps(transform, other_pad, &incoming, &fixated);
		if (ret != 0) {
			return ret;
		}

		*event->caps = fixated;
		ret = mpipe_pad_send_event(other_pad->peer, event);

		/* Apply after forwarding: set_caps() may unlink the element */
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

int mpipe_transform_change_state(struct mpipe_element *self, enum mpipe_state_change transition)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)self;

	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_element_reset_pad_caps(self);

		/* Stop the pool this element started, or it cannot be reconfigured next run */
		if (transform->out_pool != NULL) {
			(void)mpipe_buffer_pool_stop(transform->out_pool);
		}
		break;
	default:
		break;
	}

	return 0;
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

	mpipe_pad_init(&transform->sink_pad, SINK_PAD_ID, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);
	mpipe_pad_init(&transform->src_pad, SRC_PAD_ID, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &transform->sink_pad);
	mpipe_element_add_pad(self, &transform->src_pad);

	self->change_state = mpipe_transform_change_state;

	transform->mode = MPIPE_TRANSFORM_MODE_PASSTHROUGH;
	transform->set_caps = mpipe_transform_set_caps;
	transform->transform_caps = mpipe_transform_transform_caps;
	transform->sink_pad.process_fn = mpipe_transform_process_fn;
	transform->sink_pad.query_fn = mpipe_transform_query;
	transform->src_pad.query_fn = mpipe_transform_query;
	transform->sink_pad.event_fn = mpipe_transform_event;
	transform->src_pad.event_fn = mpipe_transform_event;
	transform->decide_buffer_pool = NULL;
	transform->propose_buffer_pool = NULL;

	return 0;
}
