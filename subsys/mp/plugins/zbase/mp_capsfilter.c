/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/mp/zbase/mp_capsfilter.h>

int mp_caps_filter_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_transform *transform = (struct mp_transform *)obj;

	switch (key) {
	case PROP_CAPS:
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
	case PROP_CAPS:
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
	 *  - More importantly, allow allocation negotiation can take place between the elements
	 *    before and after the capsfilter.
	 *
	 * Remember to re-insert it to the pipeline whenever caps negotiation is re-triggered.
	 */
	if (upstream_srcpad != NULL && downstream_sinkpad != NULL) {
		upstream_srcpad->peer = downstream_sinkpad;
		downstream_sinkpad->peer = upstream_srcpad;
	}

	/*
	 * Drop the peer links to avoid cycling graph error when walking through the pipeline graph
	 * TODO: Store the peer links somewhere to be able to re-insert the capsfilter when caps
	 * negotiation re-triggered
	 */
	transform->sinkpad.peer = NULL;
	transform->srcpad.peer = NULL;

	return 0;
}

void mp_caps_filter_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;

	/* Init base class */
	mp_transform_init(self);

	self->object.set_property = mp_caps_filter_set_property;
	self->object.get_property = mp_caps_filter_get_property;

	transform->mode = MP_MODE_PASSTHROUGH;
	transform->set_caps = mp_caps_filter_set_caps;
}
