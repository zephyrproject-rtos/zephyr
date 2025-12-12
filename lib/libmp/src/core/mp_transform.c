/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_transform.h"

LOG_MODULE_REGISTER(mp_transform, CONFIG_LIBMP_LOG_LEVEL);

#define MP_PAD_SINK_ID 0
#define MP_PAD_SRC_ID  1

int mp_transform_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	return 0;
}

int mp_transform_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	return 0;
}

static bool mp_transform_chainfn(struct mp_pad *pad, struct mp_buffer *buffer)
{
	struct mp_transform *transform = MP_TRANSFORM(pad->object.container);

	/* Default implementation for MP_MODE_PASSTHROUGH */
	return mp_pad_push(&transform->srcpad, buffer);
}

static struct mp_caps *mp_transform_get_caps(struct mp_transform *transform,
					     enum mp_pad_direction direction)
{
	/*
	 * Default implementation simply returns the pad's caps. However, subclasses
	 * should return the real caps of the element.
	 */
	if (direction == MP_PAD_SRC) {
		return mp_caps_ref(transform->srcpad.caps);
	}
	if (direction == MP_PAD_SINK) {
		return mp_caps_ref(transform->sinkpad.caps);
	}

	return NULL;
}

bool mp_transform_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
			   struct mp_caps *caps)
{
	if (direction == MP_PAD_SINK) {
		mp_caps_replace(&(transform->sinkpad.caps), caps);
	} else if (direction == MP_PAD_SRC) {
		mp_caps_replace(&(transform->srcpad.caps), caps);
	} else {
		return false;
	}

	return true;
}

static struct mp_caps *mp_transform_transform_caps(struct mp_transform *self,
						   enum mp_pad_direction direction,
						   struct mp_caps *incaps)
{
	return mp_caps_ref(incaps);
}

static inline bool mp_transform_query_caps(struct mp_transform *self,
					   enum mp_pad_direction direction, struct mp_query *query)
{
	int ret = false;
	struct mp_pad *this_pad, *other_pad;
	struct mp_caps *queried_pad_caps, *transformed_caps, *query_caps, *query_back_caps,
		*res_caps;

	switch (direction) {
	case MP_PAD_SINK:
		this_pad = &self->sinkpad;
		other_pad = &self->srcpad;
		break;
	case MP_PAD_SRC:
		this_pad = &self->srcpad;
		other_pad = &self->sinkpad;
		break;
	default:
		return false;
	}

	/* Intersect the query caps with the pad's caps */
	queried_pad_caps = mp_caps_intersect(mp_query_get_caps(query), this_pad->caps);
	if (queried_pad_caps == NULL) {
		return false;
	}

	transformed_caps = self->transform_caps(self, other_pad->direction, queried_pad_caps);
	if (transformed_caps == NULL) {
		return false;
	}

	/* Query the peer pad with the transformed caps */
	ret = mp_query_set_caps(query, transformed_caps);
	mp_caps_unref(transformed_caps);
	if (!ret) {
		return false;
	}

	if (!mp_pad_query(other_pad->peer, query)) {
		mp_caps_unref(queried_pad_caps);
		return false;
	}

	query_caps = mp_query_get_caps(query);

	/*
	 * Keep query_caps result at other_pad to use later at caps event.
	 * It is needed to intersect with the negotiated / fixated caps in the
	 * caps event because when passing through the transform_caps() the
	 * fixated caps will become unfixated.
	 */
	mp_caps_replace(&other_pad->caps, query_caps);

	/* Transform back the query_caps */
	query_back_caps = self->transform_caps(self, this_pad->direction, query_caps);

	/* Intersect with the original queried_pad_caps */
	res_caps = mp_caps_intersect(query_back_caps, queried_pad_caps);
	mp_caps_unref(queried_pad_caps);
	mp_caps_unref(query_back_caps);

	/* Answer the upstream query */
	ret = mp_query_set_caps(query, res_caps);
	mp_caps_unref(res_caps);

	return ret;
}

static bool mp_transform_decide_allocation(struct mp_transform *self, struct mp_query *query)
{
	return true;
}

static bool mp_transform_propose_allocation(struct mp_transform *self, struct mp_query *query)
{
	return true;
}

static bool mp_transform_query(struct mp_pad *pad, struct mp_query *query)
{
	struct mp_transform *self = MP_TRANSFORM(pad->object.container);
	int ret = false;

	switch (query->type) {
	case MP_QUERY_CAPS:
		return mp_transform_query_caps(self, pad->direction, query);
	case MP_QUERY_ALLOCATION:
		struct mp_query *peer_query = mp_query_new_allocation(self->srcpad.caps);

		/* Query the downstream */
		if (!mp_pad_query(self->srcpad.peer, peer_query)) {
			mp_query_destroy(peer_query);
			return false;
		}

		/* Decide allocation for downstream */
		ret = self->decide_allocation(self, peer_query);
		mp_query_destroy(peer_query);
		if (!ret) {
			return false;
		}

		if (self->mode == MP_MODE_NORMAL) {
			if (self->outpool == NULL) {
				return false;
			}

			/* Configure the output buffer pool with negotiated configs */
			if (!self->outpool->configure(
				    self->outpool, mp_caps_get_structure(self->srcpad.caps, 0))) {
				return false;
			}

			/* Start the output buffer pool */
			if (!self->outpool->start(self->outpool)) {
				return false;
			}
		}

		/* Propose allocation to upstream */
		return self->propose_allocation(self, query);
	default:
		return false;
	}
}

static bool mp_transform_event(struct mp_pad *pad, struct mp_event *event)
{
	bool ret = false;

	switch (event->type) {
	case MP_EVENT_EOS:
		LOG_DBG("MP_EVENT_EOS");
		return mp_pad_send_event_default(pad, event);
	case MP_EVENT_CAPS:
		LOG_DBG("MP_EVENT_CAPS");
		struct mp_transform *transform = MP_TRANSFORM(pad->object.container);
		struct mp_pad *other_pad;
		struct mp_caps *event_caps, *transformed_caps, *intersect_caps, *fixated_caps;

		other_pad =
			(pad->direction == MP_PAD_SINK) ? &transform->srcpad : &transform->sinkpad;

		event_caps = mp_event_get_caps(event);
		if (event_caps == NULL) {
			return false;
		}

		transformed_caps =
			transform->transform_caps(transform, other_pad->direction, event_caps);
		if (transformed_caps == NULL) {
			return false;
		}

		/*
		 * Intersect the transformed caps with the current other_pad's caps which stores the
		 * downstream query caps result during caps query phase
		 */
		intersect_caps = mp_caps_intersect(transformed_caps, other_pad->caps);
		mp_caps_unref(transformed_caps);
		if (intersect_caps == NULL) {
			return false;
		}

		/* Fixate the result */
		fixated_caps = mp_caps_fixate(intersect_caps);
		mp_caps_unref(intersect_caps);
		if (fixated_caps == NULL) {
			return false;
		}

		if (!mp_event_set_caps(event, fixated_caps)) {
			mp_caps_unref(fixated_caps);
			return false;
		}

		if (!mp_pad_send_event(other_pad->peer, event)) {
			mp_caps_unref(fixated_caps);
			return false;
		}

		if (!transform->set_caps(transform, pad->direction, event_caps)) {
			return false;
		}

		ret = transform->set_caps(transform, other_pad->direction, fixated_caps);
		mp_caps_unref(fixated_caps);
		return ret;
	default:
		return ret;
	}
}

void mp_transform_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);

	/* Add pads */
	mp_pad_init(&transform->sinkpad, MP_PAD_SINK_ID, MP_PAD_SINK, MP_PAD_ALWAYS, NULL);
	mp_pad_init(&transform->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(self, &transform->sinkpad);
	mp_element_add_pad(self, &transform->srcpad);

	self->object.set_property = mp_transform_set_property;
	self->object.get_property = mp_transform_get_property;

	transform->mode = MP_MODE_PASSTHROUGH;
	transform->get_caps = mp_transform_get_caps;
	transform->set_caps = mp_transform_set_caps;
	transform->transform_caps = mp_transform_transform_caps;
	transform->sinkpad.chainfn = mp_transform_chainfn;
	transform->sinkpad.queryfn = mp_transform_query;
	transform->srcpad.queryfn = mp_transform_query;
	transform->sinkpad.eventfn = mp_transform_event;
	transform->srcpad.eventfn = mp_transform_event;
	transform->decide_allocation = mp_transform_decide_allocation;
	transform->propose_allocation = mp_transform_propose_allocation;
}
