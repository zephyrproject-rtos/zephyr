/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_pad.h"
#include "mp_property.h"
#include "mp_src.h"

LOG_MODULE_REGISTER(mp_src, CONFIG_LIBMP_LOG_LEVEL);

#define MP_PAD_SRC_ID 0

int mp_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_src *src = MP_SRC(obj);

	switch (key) {
	case PROP_NUM_BUFS:
		src->num_buffers = (uint32_t)(uintptr_t)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

int mp_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_src *src = MP_SRC(obj);

	switch (key) {
	case PROP_NUM_BUFS:
		*(uint32_t *)val = src->num_buffers;
		break;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}

	return 0;
}

static bool mp_src_query(struct mp_pad *pad, struct mp_query *query)
{
	bool ret = false;
	struct mp_src *src = MP_SRC(pad->object.container);
	struct mp_caps *intersect_caps, *query_caps;

	switch (query->type) {
	case MP_QUERY_CAPS:
		query_caps = mp_query_get_caps(query);
		if (query_caps != NULL) {
			intersect_caps = mp_caps_intersect(src->srcpad.caps, query_caps);
			ret = mp_query_set_caps(query, intersect_caps);
			mp_caps_unref(intersect_caps);
		} else {
			ret = mp_query_set_caps(query, src->srcpad.caps);
		}
		break;
	default:
		ret = mp_pad_query(pad->peer, query);
		break;
	}

	return ret;
}

static bool mp_src_decide_allocation(struct mp_src *self, struct mp_query *query)
{
	return true;
}

static bool mp_src_negotiate(struct mp_src *src)
{
	bool ret = false;
	struct mp_caps *fixated_caps;
	struct mp_query *caps_query, *alloc_query;
	struct mp_event *caps_event;

	/* Caps negotiation */
	if (src->srcpad.caps == NULL) {
		return false;
	}

	/* Query the peer's capabilities */
	caps_query = mp_query_new_caps(src->srcpad.caps);
	if (!mp_pad_query(src->srcpad.peer, caps_query)) {
		mp_query_destroy(caps_query);
		return false;
	}

	/* Fixate the common caps */
	fixated_caps = mp_caps_fixate(mp_query_get_caps(caps_query));
	mp_query_destroy(caps_query);
	if (fixated_caps == NULL) {
		return false;
	}

	/* Send caps event to configure downstream */
	caps_event = mp_event_new_caps(fixated_caps);
	ret = mp_pad_send_event(src->srcpad.peer, caps_event);
	if (ret && !src->set_caps(src, fixated_caps)) {
		mp_caps_unref(fixated_caps);
		mp_event_destroy(caps_event);
		return false;
	}
	mp_caps_unref(fixated_caps);
	mp_event_destroy(caps_event);

	/* Query the peer's allocation proposal */
	alloc_query = mp_query_new_allocation(src->srcpad.caps);
	if (!mp_pad_query(src->srcpad.peer, alloc_query)) {
		mp_query_destroy(alloc_query);
		return false;
	}

	/* Decide the allocation */
	ret = src->decide_allocation(src, alloc_query);
	mp_query_destroy(alloc_query);

	return ret;
}

static enum mp_state_change_return mp_src_change_state(struct mp_element *self,
						       enum mp_state_change transition)
{
	struct mp_src *src = MP_SRC(self);
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		/* Perform negotiation */
		if (MP_OBJECT(&src->srcpad)->flags & MP_PAD_FLAG_NEGOTIATE) {
			if (!mp_src_negotiate(src)) {
				LOG_ERR("Negotiation failed");
				return MP_STATE_CHANGE_FAILURE;
			}

			/* Config buffer pool */
			if (src->srcpad.caps == NULL) {
				LOG_ERR("No source pad caps to configure buffer pool");
				return MP_STATE_CHANGE_FAILURE;
			}

			if (!src->pool->configure(src->pool,
						  mp_caps_get_structure(src->srcpad.caps, 0))) {
				LOG_ERR("Failed to configure buffer pool");
				return MP_STATE_CHANGE_FAILURE;
			}

			/* Start buffer pool */
			src->pool->start(src->pool);

			/* Clear MP_PAD_FLAG_NEGOTIATE flag */
			MP_OBJECT(&src->srcpad)->flags &= ~MP_PAD_FLAG_NEGOTIATE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		break;
	default:
		break;
	}

	return ret;
}

static bool mp_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	return true;
}

void mp_src_init(struct mp_element *self)
{
	struct mp_src *src = MP_SRC(self);

	/* Add pad */
	mp_pad_init(&src->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(self, &src->srcpad);

	self->object.set_property = mp_src_set_property;
	self->object.get_property = mp_src_get_property;
	self->change_state = mp_src_change_state;

	src->set_caps = mp_src_set_caps;
	src->srcpad.queryfn = mp_src_query;
	src->decide_allocation = mp_src_decide_allocation;

	/* Set MP_PAD_FLAG_NEGOTIATE flag */
	MP_OBJECT(&src->srcpad)->flags |= MP_PAD_FLAG_NEGOTIATE;
}
