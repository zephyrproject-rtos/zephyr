/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_transform.h"

LOG_MODULE_REGISTER(mp_transform, CONFIG_LIBMP_LOG_LEVEL);

int mp_transform_set_property(MpObject *obj, uint32_t key, const void *val)
{
	return 0;
}

int mp_transform_get_property(MpObject *obj, uint32_t key, void *val)
{
	return 0;
}

static bool mp_transform_chainfn(MpPad *pad, MpBuffer *buffer)
{
	MpTransform *transform = MP_TRANSFORM(pad->object.container);

	/* Doing all transform tasks here */

	/* Push buffer to src pad */
	return mp_pad_push(&transform->srcpad, buffer);
}

static MpCaps *mp_transform_transform_caps(MpTransform *self, MpPadDirection direction,
					   MpCaps *incaps)
{
	return self->transform_caps ? self->transform_caps(self, direction, incaps) : NULL;
}

static inline bool mp_transform_query_caps(MpTransform *self, MpPadDirection direction, MpQuery *query)
{
	int ret = false;
	MpPad *this_pad, *other_pad;
	MpCaps *queried_pad_caps, *transformed_caps, *query_caps, *query_back_caps, *res_caps;

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

	transformed_caps =
		mp_transform_transform_caps(self, other_pad->direction, queried_pad_caps);
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
	query_back_caps = mp_transform_transform_caps(self, this_pad->direction, query_caps);

	/* Intersect with the original queried_pad_caps */
	res_caps = mp_caps_intersect(query_back_caps, queried_pad_caps);
	mp_caps_unref(queried_pad_caps);
	mp_caps_unref(query_back_caps);

	/* Answer the upstream query */
	ret = mp_query_set_caps(query, res_caps);
	mp_caps_unref(res_caps);

	return ret;
}

static bool mp_transform_decide_allocation_default(MpTransform *self, MpQuery *query)
{
	return true;
}

static bool mp_transform_propose_allocation_default(MpTransform *self, MpQuery *query)
{
	return true;
}

static bool mp_transform_query(MpPad *pad, MpQuery *query)
{
	MpTransform *self = MP_TRANSFORM(pad->object.container);
	int ret = false;

	switch (query->type) {
	case MP_QUERY_CAPS:
		return mp_transform_query_caps(self, pad->direction, query);
	case MP_QUERY_ALLOCATION:
		MpQuery *peer_query = mp_query_new_allocation(self->srcpad.caps);

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
			if (!self->outpool->configure(self->outpool,
							mp_caps_get_structure(self->srcpad.caps, 0))) {
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

static bool mp_transform_event(MpPad *pad, MpEvent *event)
{
	bool ret = false;

	switch (event->type) {
	case MP_EVENT_EOS:
		LOG_DBG("MP_EVENT_EOS");
		return mp_pad_send_event_default(pad, event);
	case MP_EVENT_CAPS:
		LOG_DBG("MP_EVENT_CAPS");
		MpTransform *transform = MP_TRANSFORM(pad->object.container);
		MpPad *other_pad;
		MpCaps *event_caps, *transformed_caps, *intersect_caps, *fixated_caps;

		other_pad =
			(pad->direction == MP_PAD_SINK) ? &transform->srcpad : &transform->sinkpad;

		event_caps = mp_event_get_caps(event);
		if (event_caps == NULL) {
			return false;
		}

		transformed_caps = transform->transform_caps(transform, other_pad->direction, event_caps);
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

void mp_transform_init(MpElement *self)
{
	MpTransform *transform = MP_TRANSFORM(self);

	/* Add pads */
	mp_pad_init(&transform->sinkpad, "sink", MP_PAD_SINK, MP_PAD_ALWAYS, NULL);
	mp_pad_init(&transform->srcpad, "src", MP_PAD_SRC, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(self, &transform->sinkpad);
	mp_element_add_pad(self, &transform->srcpad);

	self->object.set_property = mp_transform_set_property;
	self->object.get_property = mp_transform_get_property;

	transform->sinkpad.chainfn = mp_transform_chainfn;
	transform->sinkpad.queryfn = mp_transform_query;
	transform->srcpad.queryfn = mp_transform_query;
	transform->sinkpad.eventfn = mp_transform_event;
	transform->srcpad.eventfn = mp_transform_event;
	transform->decide_allocation = mp_transform_decide_allocation_default;
	transform->propose_allocation = mp_transform_propose_allocation_default;
}
