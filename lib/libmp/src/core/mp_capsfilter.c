/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mp_capsfilter.h"

int mp_caps_filter_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_transform *transform = MP_TRANSFORM(obj);

	switch (key) {
	case PROP_CAPS:
		mp_caps_replace(&transform->sinkpad.caps, (struct mp_caps *)val);
		mp_caps_replace(&transform->srcpad.caps, (struct mp_caps *)val);
		return 0;
	default:
		return mp_transform_set_property(obj, key, val);
	}
}

int mp_caps_filter_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_transform *transform = MP_TRANSFORM(obj);

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
		return mp_transform_get_property(obj, key, val);
	}
}

static bool mp_caps_filter_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
				    struct mp_caps *caps)
{
	if (!mp_transform_set_caps(transform, direction, caps)) {
		return false;
	}

	/*
	 * After caps negotiation, capsfilter is bypassed in the pipeline for two reasons:
	 *  - Gain some small overhead during buffer flow
	 *  - More importanly, allow allocation negotiation can take place between the elements
	 *    before and after the capsfilter.
	 *
	 * TODO: Remember to remove this shortcut whenever caps negotiation is re-triggered.
	 */
	transform->sinkpad.peer->peer = transform->srcpad.peer;

	return true;
}

void mp_caps_filter_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);

	/* Init base class */
	mp_transform_init(self);

	self->object.set_property = mp_caps_filter_set_property;
	self->object.get_property = mp_caps_filter_get_property;

	transform->mode = MP_MODE_PASSTHROUGH;
	transform->set_caps = mp_caps_filter_set_caps;
	/* All-pass filter is set by default */
	transform->sinkpad.caps = mp_caps_new_any();
	transform->srcpad.caps = mp_caps_new_any();
}
