/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mpipe/base/mpipe_caps_filter.h>

int mpipe_caps_filter_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)obj;
	struct mpipe_caps_filter *filter = (struct mpipe_caps_filter *)obj;

	switch (key) {
	case MPIPE_PROP_BASE_CAPS_FILTER_CAPS:
		filter->filter_caps = *(const struct mpipe_structure *)val;
		mpipe_pad_set_caps(&transform->sink_pad, &filter->filter_caps);
		mpipe_pad_set_caps(&transform->src_pad, &filter->filter_caps);
		return 0;
	default:
		return -ENOTSUP;
	}
}

int mpipe_caps_filter_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_caps_filter *filter = (struct mpipe_caps_filter *)obj;

	switch (key) {
	case MPIPE_PROP_BASE_CAPS_FILTER_CAPS:
		*(struct mpipe_structure **)val = &filter->filter_caps;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_caps_filter_set_caps(struct mpipe_transform *transform,
				      enum mpipe_pad_direction direction,
				      const struct mpipe_structure *caps)
{
	struct mpipe_caps_filter *filter = (struct mpipe_caps_filter *)transform;
	int ret;
	struct mpipe_pad *upstream_src_pad = transform->sink_pad.peer;
	struct mpipe_pad *downstream_sink_pad = transform->src_pad.peer;

	ret = mpipe_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Take itself out of the graph once negotiated: buffers skip it and the pool
	 * query reaches its neighbors. The saved peers put it back on PAUSED -> READY.
	 */
	if (upstream_src_pad != NULL && downstream_sink_pad != NULL) {
		filter->saved_sink_peer = upstream_src_pad;
		filter->saved_src_peer = downstream_sink_pad;

		mpipe_pad_link(upstream_src_pad, downstream_sink_pad);
	}

	/* Drop the peer links, or the topological sort sees a cycle */
	transform->sink_pad.peer = NULL;
	transform->src_pad.peer = NULL;

	return 0;
}

static int mpipe_caps_filter_change_state(struct mpipe_element *self,
					  enum mpipe_state_change transition)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)self;
	struct mpipe_caps_filter *filter = (struct mpipe_caps_filter *)self;
	int ret;

	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		/* Put the element back so the next negotiation walks through it */
		if (filter->saved_sink_peer != NULL && filter->saved_src_peer != NULL) {
			mpipe_pad_link(filter->saved_sink_peer, &transform->sink_pad);
			mpipe_pad_link(&transform->src_pad, filter->saved_src_peer);

			filter->saved_sink_peer = NULL;
			filter->saved_src_peer = NULL;
		}
		break;
	default:
		break;
	}

	ret = mpipe_transform_change_state(self, transition);

	/* The base reset wiped the pads; the configured filter is not a negotiation result */
	if (transition == MPIPE_STATE_CHANGE_PAUSED_TO_READY) {
		mpipe_pad_set_caps(&transform->sink_pad, &filter->filter_caps);
		mpipe_pad_set_caps(&transform->src_pad, &filter->filter_caps);
	}

	return ret;
}

int mpipe_caps_filter_init(struct mpipe_caps_filter *caps_filter, uint8_t id)
{
	__ASSERT_NO_MSG(caps_filter != NULL);

	struct mpipe_element *self = &caps_filter->transform.element;
	struct mpipe_transform *transform = &caps_filter->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "caps_filter");

	ret = mpipe_structure_init_any(&caps_filter->filter_caps);
	if (ret != 0) {
		return ret;
	}

	self->object.set_property = mpipe_caps_filter_set_property;
	self->object.get_property = mpipe_caps_filter_get_property;
	self->change_state = mpipe_caps_filter_change_state;

	transform->mode = MPIPE_MODE_PASSTHROUGH;
	transform->set_caps = mpipe_caps_filter_set_caps;

	return 0;
}
