/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_transform.h>

LOG_MODULE_REGISTER(mp_transform, CONFIG_MP_LOG_LEVEL);

#define MP_PAD_SINK_ID 0
#define MP_PAD_SRC_ID  1

void mp_transform_update_caps(struct mp_transform *transform, struct mp_caps *sink_caps,
			      struct mp_caps *src_caps)
{
	mp_caps_replace(&transform->sink_caps, sink_caps);
	mp_caps_replace(&transform->sinkpad.caps, transform->sink_caps);
	mp_caps_replace(&transform->src_caps, src_caps);
	mp_caps_replace(&transform->srcpad.caps, transform->src_caps);
}

static int mp_transform_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	/* Default implementation for MP_MODE_PASSTHROUGH - return same buffer */
	*out_buf = in_buf;
	return 0;
}

static struct mp_caps *mp_transform_get_caps(struct mp_transform *transform,
					     enum mp_pad_direction direction)
{
	if (direction == MP_PAD_SRC) {
		return mp_caps_ref(transform->src_caps);
	}
	if (direction == MP_PAD_SINK) {
		return mp_caps_ref(transform->sink_caps);
	}

	return NULL;
}

int mp_transform_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
			  struct mp_caps *caps)
{
	if (direction == MP_PAD_SINK) {
		mp_caps_replace(&(transform->sinkpad.caps), caps);
	} else if (direction == MP_PAD_SRC) {
		mp_caps_replace(&(transform->srcpad.caps), caps);
	} else {
		return -EINVAL;
	}

	return 0;
}

static struct mp_caps *mp_transform_transform_caps(struct mp_transform *self,
						   enum mp_pad_direction direction,
						   struct mp_caps *incaps)
{
	return mp_caps_ref(incaps);
}

static inline int mp_transform_query_caps(struct mp_transform *self,
					  enum mp_pad_direction direction,
					  struct mp_dispatch *query)
{
	int ret;
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
		return -EINVAL;
	}

	/* Intersect the query caps with the pad's caps */
	query_caps = mp_dispatch_get_caps(query);
	queried_pad_caps = mp_caps_intersect(query_caps, this_pad->caps);
	mp_caps_unref(query_caps);
	if (queried_pad_caps == NULL) {
		return -ENODATA;
	}
	if (mp_caps_is_empty(queried_pad_caps)) {
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}

	transformed_caps = self->transform_caps(self, other_pad->direction, queried_pad_caps);
	if (transformed_caps == NULL) {
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}
	if (mp_caps_is_empty(transformed_caps)) {
		mp_caps_unref(transformed_caps);
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}

	/* Query the peer pad with the transformed caps */
	ret = mp_dispatch_set_caps(query, transformed_caps);
	mp_caps_unref(transformed_caps);
	if (ret < 0) {
		mp_caps_unref(queried_pad_caps);
		return ret;
	}

	ret = mp_pad_query(other_pad->peer, query);
	if (ret < 0) {
		LOG_ERR("error element id = %u", self->element.object.id);
		mp_caps_unref(queried_pad_caps);
		return ret;
	}

	query_caps = mp_dispatch_get_caps(query);
	if (query_caps == NULL || mp_caps_is_empty(query_caps)) {
		mp_caps_unref(queried_pad_caps);
		mp_caps_unref(query_caps);
		return -ENODATA;
	}

	/*
	 * Keep query_caps result at other_pad to use later at caps event.
	 * It is needed to intersect with the negotiated / fixated caps in the
	 * caps event because when passing through the transform_caps() the
	 * fixated caps will become unfixated.
	 */
	mp_caps_replace(&other_pad->caps, query_caps);

	/* Transform back the query_caps */
	query_back_caps = self->transform_caps(self, this_pad->direction, query_caps);
	mp_caps_unref(query_caps);
	if (query_back_caps == NULL) {
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}
	if (mp_caps_is_empty(query_back_caps)) {
		mp_caps_unref(query_back_caps);
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}

	/* Intersect with the original queried_pad_caps */
	res_caps = mp_caps_intersect(query_back_caps, queried_pad_caps);
	mp_caps_unref(queried_pad_caps);
	mp_caps_unref(query_back_caps);

	if (res_caps == NULL) {
		return -ENODATA;
	}
	if (mp_caps_is_empty(res_caps)) {
		mp_caps_unref(res_caps);
		return -ENODATA;
	}

	/* Answer the upstream query */
	ret = mp_dispatch_set_caps(query, res_caps);
	mp_caps_unref(res_caps);

	return ret;
}

static int mp_transform_query(struct mp_pad *pad, struct mp_dispatch *query)
{
	struct mp_transform *self = (struct mp_transform *)pad->object.container;
	int ret;

	switch (query->type) {
	case MP_DISPATCH_CAPS:
		return mp_transform_query_caps(self, pad->direction, query);
	case MP_DISPATCH_BUFFER_CONFIG:
		struct mp_dispatch peer_query;

		mp_dispatch_buffer_config_init(&peer_query, self->srcpad.caps);

		/* Query the downstream */
		ret = mp_pad_query(self->srcpad.peer, &peer_query);
		if (ret < 0) {
			mp_dispatch_clear(&peer_query);
			return ret;
		}

		/* Decide allocation for downstream */
		if (self->decide_allocation != NULL) {
			ret = self->decide_allocation(self, &peer_query);
			if (ret < 0) {
				mp_dispatch_clear(&peer_query);
				return ret;
			}
		}

		mp_dispatch_clear(&peer_query);

		/* Configure/start the output buffer pool */
		if (self->mode == MP_MODE_NORMAL) {
			ret = mp_buffer_pool_configure(self->outpool,
						       mp_caps_get_structure(self->srcpad.caps, 0));
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to configure output transform buffer pool");
				return ret;
			}

			ret = mp_buffer_pool_start(self->outpool);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to start output transform buffer pool");
				return ret;
			}
		}

		/* Propose allocation to upstream */
		if (self->propose_allocation != NULL) {
			return self->propose_allocation(self, query);
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_transform_event(struct mp_pad *pad, struct mp_dispatch *event)
{
	int ret;

	switch (event->type) {
	case MP_DISPATCH_EOS:
		LOG_DBG("MP_DISPATCH_EOS");
		return mp_pad_send_event_default(pad, event);
	case MP_DISPATCH_CAPS:
		LOG_DBG("MP_DISPATCH_CAPS");
		struct mp_transform *transform = (struct mp_transform *)pad->object.container;
		struct mp_pad *other_pad;
		struct mp_caps *event_caps, *transformed_caps, *intersect_caps, *fixated_caps;

		other_pad =
			(pad->direction == MP_PAD_SINK) ? &transform->srcpad : &transform->sinkpad;

		event_caps = mp_dispatch_get_caps(event);
		if (event_caps == NULL) {
			return -EINVAL;
		}

		transformed_caps =
			transform->transform_caps(transform, other_pad->direction, event_caps);
		if (transformed_caps == NULL) {
			mp_caps_unref(event_caps);
			return -ENODATA;
		}

		/*
		 * Intersect the transformed caps with the current other_pad's caps which stores the
		 * downstream query caps result during caps query phase
		 */
		intersect_caps = mp_caps_intersect(transformed_caps, other_pad->caps);
		mp_caps_unref(transformed_caps);
		if (intersect_caps == NULL) {
			return -ENODATA;
		}

		/* Fixate the result */
		fixated_caps = mp_caps_fixate(intersect_caps);
		mp_caps_unref(intersect_caps);
		if (fixated_caps == NULL) {
			return -ENODATA;
		}

		ret = mp_dispatch_set_caps(event, fixated_caps);
		if (ret < 0) {
			mp_caps_unref(fixated_caps);
			mp_caps_unref(event_caps);
			return ret;
		}

		ret = mp_pad_send_event(other_pad->peer, event);
		if (ret < 0) {
			mp_caps_unref(fixated_caps);
			mp_caps_unref(event_caps);
			return ret;
		}

		ret = transform->set_caps(transform, pad->direction, event_caps);
		if (ret < 0) {
			mp_caps_unref(fixated_caps);
			mp_caps_unref(event_caps);
			return ret;
		}

		ret = transform->set_caps(transform, other_pad->direction, fixated_caps);
		mp_caps_unref(fixated_caps);
		mp_caps_unref(event_caps);

		return ret;
	default:
		return -ENOTSUP;
	}
}

void mp_transform_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;

	/* Default supported caps */
	transform->sink_caps = mp_caps_new_any();
	transform->src_caps = mp_caps_new_any();

	mp_pad_init(&transform->sinkpad, MP_PAD_SINK_ID, MP_PAD_SINK, MP_PAD_ALWAYS,
		    transform->sink_caps);
	mp_pad_init(&transform->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS,
		    transform->src_caps);
	mp_element_add_pad(self, &transform->sinkpad);
	mp_element_add_pad(self, &transform->srcpad);

	transform->mode = MP_MODE_PASSTHROUGH;
	transform->get_caps = mp_transform_get_caps;
	transform->set_caps = mp_transform_set_caps;
	transform->transform_caps = mp_transform_transform_caps;
	transform->sinkpad.chainfn = mp_transform_chainfn;
	transform->sinkpad.queryfn = mp_transform_query;
	transform->srcpad.queryfn = mp_transform_query;
	transform->sinkpad.eventfn = mp_transform_event;
	transform->srcpad.eventfn = mp_transform_event;
	transform->decide_allocation = NULL;
	transform->propose_allocation = NULL;
}
