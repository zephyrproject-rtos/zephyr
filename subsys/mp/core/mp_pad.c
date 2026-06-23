/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

LOG_MODULE_REGISTER(mp_pad, CONFIG_MP_LOG_LEVEL);

void mp_pad_init(struct mp_pad *pad, uint8_t id, enum mp_pad_direction direction,
		 enum mp_pad_presence presence, struct mp_caps *caps)
{
	__ASSERT_NO_MSG(pad != NULL);

	pad->object.id = id;
	pad->direction = direction;
	pad->presence = presence;
	mp_caps_replace(&pad->caps, caps);
}

struct mp_pad *mp_pad_new(uint8_t id, enum mp_pad_direction direction,
			  enum mp_pad_presence presence, struct mp_caps *caps)
{
	struct mp_pad *pad = k_calloc(1, sizeof(struct mp_pad));

	if (pad == NULL) {
		return NULL;
	}

	mp_pad_init(pad, id, direction, presence, caps);

	return pad;
}

int mp_pad_link(struct mp_pad *srcpad, struct mp_pad *sinkpad)
{
	if (srcpad == NULL || sinkpad == NULL) {
		return -EINVAL;
	}

	/* Set peer pad */
	srcpad->peer = sinkpad;
	sinkpad->peer = srcpad;

	return 0;
}

int mp_pad_query(struct mp_pad *pad, struct mp_dispatch *query)
{
	int ret;

	if (pad == NULL || query == NULL) {
		return -EINVAL;
	}

	if (pad->queryfn == NULL) {
		return -ENOTSUP;
	}

	ret = pad->queryfn(pad, query);
	if (ret != 0) {
		return ret;
	}

	/* Caps query is considered successful only if the query's caps is valid */
	if (query->type == MP_DISPATCH_CAPS) {
		struct mp_caps *query_caps = mp_dispatch_get_caps(query);

		if (query_caps == NULL) {
			return -ENODATA;
		}

		if (mp_caps_is_empty(query_caps)) {
			mp_caps_unref(query_caps);
			return -ENODATA;
		}

		mp_caps_unref(query_caps);
	}

	return 0;
}

int mp_pad_send_event_default(struct mp_pad *pad, struct mp_dispatch *event)
{
	int ret = -ENOTSUP;

	if (pad == NULL || event == NULL) {
		return -EINVAL;
	}

	struct mp_element *element = (struct mp_element *)pad->object.container;
	struct mp_object *obj;
	sys_dlist_t *otherpad_list;

	if (pad->direction == MP_PAD_SINK) {
		otherpad_list = &element->srcpads;
	} else {
		otherpad_list = &element->sinkpads;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(otherpad_list, obj, node) {
		struct mp_pad *otherpad = (struct mp_pad *)obj;

		if (otherpad->peer == NULL) {
			LOG_DBG("pad %u: no peer, skipping", obj->id);
			continue;
		}

		int r = mp_pad_send_event(otherpad->peer, event);

		if (r == 0) {
			ret = 0;
		} else {
			LOG_DBG("pad %u: event send to peer failed: %d", obj->id, r);
		}
	}

	return ret;
}

int mp_pad_send_event(struct mp_pad *pad, struct mp_dispatch *event)
{
	if (pad == NULL || event == NULL) {
		return -EINVAL;
	}

	if (pad->eventfn == NULL) {
		return -ENOTSUP;
	}

	return pad->eventfn(pad, event);
}
