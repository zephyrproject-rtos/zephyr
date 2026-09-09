/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>

LOG_MODULE_REGISTER(mpipe_pad, CONFIG_MPIPE_LOG_LEVEL);

int mpipe_pad_enum_filter(const struct mpipe_structure *candidate,
			  const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	__ASSERT_NO_MSG(candidate != NULL);
	__ASSERT_NO_MSG(out != NULL);

	if (filter == NULL) {
		*out = *candidate;
		return 0;
	}

	/* This capability cannot satisfy the filter, but a later one might */
	return (mpipe_structure_intersect(candidate, filter, out) != 0) ? -EAGAIN : 0;
}

static int mpipe_pad_enum_caps_default(struct mpipe_pad *pad, uint32_t index,
				       const struct mpipe_structure *filter,
				       struct mpipe_structure *out)
{
	/* A pad carries one capability, so there is nothing past index 0 */
	if (index > 0U) {
		return -ENOENT;
	}

	return mpipe_pad_enum_filter(&pad->caps, filter, out);
}

int mpipe_pad_enum_caps(struct mpipe_pad *pad, uint32_t index, const struct mpipe_structure *filter,
			struct mpipe_structure *out)
{
	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(out != NULL);

	if (pad->enum_caps_fn == NULL) {
		return -EINVAL;
	}

	/*
	 * Every search over the indices ends on -ENOENT, so one that is never reported
	 * would run forever. Report it here rather than trust each element.
	 */
	if (index > UINT16_MAX) {
		LOG_ERR("pad id = %u: enum_caps_fn never reported the end of its capabilities",
			pad->object.id);
		return -ENOENT;
	}

	return pad->enum_caps_fn(pad, index, filter, out);
}

int mpipe_pad_answer_caps_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	__ASSERT_NO_MSG(query != NULL);

	struct mpipe_structure candidate;
	int ret;

	/* A caps query must carry the storage the answer is written into */
	if (query->caps == NULL) {
		return -EINVAL;
	}

	ret = mpipe_pad_enum_first(pad, query->caps, &candidate);
	if (ret != 0) {
		return ret;
	}

	/* Answer the query, writing into the asker's storage */
	*query->caps = candidate;

	return 0;
}

int mpipe_pad_enum_first(struct mpipe_pad *pad, const struct mpipe_structure *filter,
			 struct mpipe_structure *out)
{
	/* A filter that constrains nothing accepts the capability as it is */
	if (filter != NULL && mpipe_structure_is_any(filter)) {
		filter = NULL;
	}

	for (uint32_t index = 0;; index++) {
		int ret = mpipe_pad_enum_caps(pad, index, filter, out);

		if (ret == -EAGAIN) {
			continue;
		}

		return (ret == -ENOENT) ? -ENODATA : ret;
	}
}

void mpipe_pad_init(struct mpipe_pad *pad, uint8_t id, enum mpipe_pad_direction direction,
		    enum mpipe_pad_presence presence)
{
	__ASSERT_NO_MSG(pad != NULL);

	mpipe_object_init(&pad->object);
	pad->object.id = id;
	pad->direction = direction;
	pad->presence = presence;
	atomic_set(&pad->flushing, 0);

	/* A pad constrains nothing until a capability is negotiated onto it */
	mpipe_structure_init_any(&pad->caps);
	pad->enum_caps_fn = mpipe_pad_enum_caps_default;
}

int mpipe_pad_set_caps(struct mpipe_pad *pad, const struct mpipe_structure *caps)
{
	__ASSERT_NO_MSG(pad != NULL);

	if (caps == &pad->caps) {
		return 0;
	}

	mpipe_structure_clear(&pad->caps);

	if (caps == NULL) {
		return mpipe_structure_init_any(&pad->caps);
	}

	pad->caps = *caps;

	return 0;
}

int mpipe_pad_link(struct mpipe_pad *src_pad, struct mpipe_pad *sink_pad)
{
	__ASSERT_NO_MSG(src_pad != NULL);
	__ASSERT_NO_MSG(sink_pad != NULL);

	/* Set peer pad */
	src_pad->peer = sink_pad;
	sink_pad->peer = src_pad;

	return 0;
}

int mpipe_pad_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	int ret;

	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(query != NULL);

	if (pad->query_fn == NULL) {
		return -ENOTSUP;
	}

	/*
	 * A caps query is answered into the storage it carries, so a query
	 * function is entitled to dereference it. Refuse one that carries none
	 * here, before any of them sees it.
	 */
	if (query->type == MPIPE_DISPATCH_CAPS && query->caps == NULL) {
		return -EINVAL;
	}

	ret = pad->query_fn(pad, query);
	if (ret != 0) {
		return ret;
	}

	/* Caps query is considered successful only if the answer is not empty */
	if (query->type == MPIPE_DISPATCH_CAPS && mpipe_structure_is_empty(query->caps)) {
		return -ENODATA;
	}

	return 0;
}

int mpipe_pad_send_event_default(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	int ret = -ENOTSUP;

	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(event != NULL);

	struct mpipe_element *element = (struct mpipe_element *)pad->object.container;
	struct mpipe_object *obj;
	sys_dlist_t *otherpad_list;

	if (pad->direction == MPIPE_PAD_SINK) {
		otherpad_list = &element->src_pads;
	} else {
		otherpad_list = &element->sink_pads;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(otherpad_list, obj, node) {
		struct mpipe_pad *otherpad = (struct mpipe_pad *)obj;

		if (otherpad->peer == NULL) {
			LOG_DBG("pad %u: no peer, skipping", obj->id);
			continue;
		}

		int r = mpipe_pad_send_event(otherpad->peer, event);

		if (r == 0) {
			ret = 0;
		} else {
			LOG_DBG("pad %u: event send to peer failed: %d", obj->id, r);
		}
	}

	return ret;
}

int mpipe_pad_send_event(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	__ASSERT_NO_MSG(pad != NULL);
	__ASSERT_NO_MSG(event != NULL);

	if (pad->event_fn == NULL) {
		return -ENOTSUP;
	}

	return pad->event_fn(pad, event);
}
