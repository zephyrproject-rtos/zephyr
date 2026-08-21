/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_src.h>

LOG_MODULE_REGISTER(mp_src, CONFIG_MP_LOG_LEVEL);

#define MP_PAD_SRC_ID 0

int mp_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_src *src = (struct mp_src *)obj;

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
	struct mp_src *src = (struct mp_src *)obj;

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

void mp_src_update_caps(struct mp_src *src, struct mp_caps *caps)
{
	mp_caps_replace(&src->src_caps, caps);
	mp_caps_replace(&src->srcpad.caps, src->src_caps);
}

static struct mp_caps *mp_src_get_caps(struct mp_src *src)
{
	return src ? mp_caps_ref(src->src_caps) : NULL;
}

static int mp_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	if (src == NULL) {
		return -EINVAL;
	}

	mp_caps_replace(&src->srcpad.caps, caps);

	return 0;
}

static int mp_src_query(struct mp_pad *pad, struct mp_dispatch *query)
{
	int ret;
	struct mp_src *src = (struct mp_src *)pad->object.container;
	struct mp_caps *intersect_caps;
	struct mp_caps *query_caps;

	switch (query->type) {
	case MP_DISPATCH_CAPS:
		query_caps = mp_dispatch_get_caps(query);
		if (query_caps != NULL) {
			intersect_caps = mp_caps_intersect(src->src_caps, query_caps);
			if (intersect_caps == NULL) {
				return -ENODATA;
			}
			if (mp_caps_is_empty(intersect_caps)) {
				mp_caps_unref(intersect_caps);
				return -ENODATA;
			}
			ret = mp_dispatch_set_caps(query, intersect_caps);
			mp_caps_unref(intersect_caps);
			mp_caps_unref(query_caps);
		} else {
			ret = mp_dispatch_set_caps(query, src->src_caps);
		}

		return ret;
	default:
		return -ENOTSUP;
	}
}

static int mp_src_negotiate(struct mp_src *src)
{
	struct mp_caps *common_caps;
	struct mp_caps *fixated_caps;
	struct mp_dispatch caps_query;
	struct mp_dispatch alloc_query;
	struct mp_dispatch caps_event;
	int ret;

	/* Caps negotiation */
	if (src->src_caps == NULL || mp_caps_is_empty(src->src_caps)) {
		return -EINVAL;
	}

	/* Query the peer's capabilities */
	mp_dispatch_caps_init(&caps_query, src->src_caps);
	ret = mp_pad_query(src->srcpad.peer, &caps_query);
	if (ret != 0) {
		mp_dispatch_clear(&caps_query);
		return ret;
	}

	common_caps = mp_dispatch_get_caps(&caps_query);
	mp_dispatch_clear(&caps_query);
	if (common_caps == NULL) {
		return -ENODATA;
	}

	if (mp_caps_is_empty(common_caps)) {
		mp_caps_unref(common_caps);
		return -ENODATA;
	}

	/* Store negotiated (possibly unfixed) caps on the src pad */
	mp_caps_replace(&src->srcpad.caps, common_caps);
	mp_caps_unref(common_caps);

	fixated_caps = mp_caps_fixate(src->srcpad.caps);

	/*
	 * Push a caps event downstream. Don't check the returned value of
	 * mp_pad_send_event() when caps is not fixatted (ANY) as we want to continue.
	 */
	mp_dispatch_caps_init(&caps_event, fixated_caps);
	ret = mp_pad_send_event(src->srcpad.peer, &caps_event);
	mp_dispatch_clear(&caps_event);

	/* Set caps if it can be fixated */
	if (fixated_caps != NULL) {
		if (ret != 0 || src->set_caps(src, fixated_caps) != 0) {
			mp_caps_unref(fixated_caps);
			return ret;
		}

		mp_caps_unref(fixated_caps);
	}

	/* Query the peer's allocation proposal */
	mp_dispatch_buffer_config_init(&alloc_query, src->srcpad.caps);
	ret = mp_pad_query(src->srcpad.peer, &alloc_query);
	if (ret != 0) {
		mp_dispatch_clear(&alloc_query);
		return ret;
	}

	/* Decide the allocation */
	if (src->decide_allocation != NULL) {
		ret = src->decide_allocation(src, &alloc_query);
		mp_dispatch_clear(&alloc_query);
		return ret;
	}

	mp_dispatch_clear(&alloc_query);

	return 0;
}

enum mp_state_change_return mp_src_change_state(struct mp_element *self,
						enum mp_state_change transition)
{
	struct mp_src *src = (struct mp_src *)self;
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;
	int pool_ret;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		/* Perform negotiation */
		if (mp_src_negotiate(src) < 0) {
			LOG_ERR("Negotiation failed");
			return MP_STATE_CHANGE_FAILURE;
		}

		/* Config buffer pool */
		pool_ret = mp_buffer_pool_configure(src->pool,
						    mp_caps_get_structure(src->srcpad.caps, 0));
		if (pool_ret != 0 && pool_ret != -ENOSYS) {
			LOG_ERR("Failed to configure source buffer pool");
			return MP_STATE_CHANGE_FAILURE;
		}

		/* Start buffer pool */
		pool_ret = mp_buffer_pool_start(src->pool);
		if (pool_ret != 0 && pool_ret != -ENOSYS) {
			LOG_ERR("Failed to start source buffer pool");
			return MP_STATE_CHANGE_FAILURE;
		}

		break;
	default:
		break;
	}

	return ret;
}

void mp_src_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;

	/* Default supported caps */
	src->src_caps = mp_caps_new_any();

	mp_pad_init(&src->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS, src->src_caps);
	mp_element_add_pad(self, &src->srcpad);

	self->object.set_property = mp_src_set_property;
	self->object.get_property = mp_src_get_property;
	self->change_state = mp_src_change_state;

	src->get_caps = mp_src_get_caps;
	src->set_caps = mp_src_set_caps;
	src->srcpad.queryfn = mp_src_query;
	src->decide_allocation = NULL;
}
