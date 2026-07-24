/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mp/base/mp_capsfilter.h>

int mp_caps_filter_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_transform *transform = (struct mp_transform *)obj;

	switch (key) {
	case MP_PROP_BASE_CAPSFILTER_CAPS:
		mp_caps_replace(&transform->sinkpad.caps, (struct mp_caps *)val);
		mp_caps_replace(&transform->srcpad.caps, (struct mp_caps *)val);
		return 0;
	default:
		return -ENOTSUP;
	}
}

int mp_caps_filter_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_transform *transform = (struct mp_transform *)obj;

	switch (key) {
	case MP_PROP_BASE_CAPSFILTER_CAPS:
		/*
		 * The pad's caps may change during and after caps negotiation but the function is
		 * generally called before any pipeline process, so it's OK to get the filter caps
		 * from the pad's caps
		 */
		val = transform->sinkpad.caps;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_caps_filter_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
				   struct mp_caps *caps)
{
	struct mp_caps_filter *filter = (struct mp_caps_filter *)transform;
	int ret;
	struct mp_pad *upstream_srcpad = transform->sinkpad.peer;
	struct mp_pad *downstream_sinkpad = transform->srcpad.peer;

	ret = mp_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	/*
	 * After caps negotiation, capsfilter is removed from the pipeline for two reasons:
	 *  - Gain some small overhead during buffer flow
	 *  - More importantly, allow allocation negotiation can take place between the
	 *    elements before and after the capsfilter.
	 *
	 * The bypassed peers are saved so the capsfilter can re-insert itself into the graph
	 * when needed, e.g. on teardown (PAUSED -> READY) or on caps re-negotiation
	 */
	if (upstream_srcpad != NULL && downstream_sinkpad != NULL) {
		filter->saved_sink_peer = upstream_srcpad;
		filter->saved_src_peer = downstream_sinkpad;

		upstream_srcpad->peer = downstream_sinkpad;
		downstream_sinkpad->peer = upstream_srcpad;
	}

	/* Drop the peer links to avoid cycling graph error */
	transform->sinkpad.peer = NULL;
	transform->srcpad.peer = NULL;

	return 0;
}

static enum mp_state_change_return mp_caps_filter_change_state(struct mp_element *self,
							       enum mp_state_change transition)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_caps_filter *filter = (struct mp_caps_filter *)self;

	switch (transition) {
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		/*
		 * Re-insert the capsfilter into the graph so that a subsequent caps
		 * negotiation (e.g. on replay) can walk through it again. This undoes
		 * the self-removal performed in mp_caps_filter_set_caps() by relinking
		 * the upstream/downstream peers back to this element's pads.
		 */
		if (filter->saved_sink_peer != NULL && filter->saved_src_peer != NULL) {
			transform->sinkpad.peer = filter->saved_sink_peer;
			transform->srcpad.peer = filter->saved_src_peer;
			filter->saved_sink_peer->peer = &transform->sinkpad;
			filter->saved_src_peer->peer = &transform->srcpad;

			filter->saved_sink_peer = NULL;
			filter->saved_src_peer = NULL;
		}
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_caps_filter_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;

	/* Init base class */
	mp_transform_init(self);

	self->object.set_property = mp_caps_filter_set_property;
	self->object.get_property = mp_caps_filter_get_property;
	self->change_state = mp_caps_filter_change_state;

	transform->mode = MP_MODE_PASSTHROUGH;
	transform->set_caps = mp_caps_filter_set_caps;
}
