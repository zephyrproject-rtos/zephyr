/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>

LOG_MODULE_REGISTER(mpipe_element, CONFIG_MPIPE_LOG_LEVEL);

void mpipe_element_reset_pad_caps(struct mpipe_element *element)
{
	__ASSERT_NO_MSG(element != NULL);

	struct mpipe_object *pad_obj;

	SYS_DLIST_FOR_EACH_CONTAINER(&element->src_pads, pad_obj, node) {
		mpipe_pad_set_caps((struct mpipe_pad *)pad_obj, NULL);
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&element->sink_pads, pad_obj, node) {
		mpipe_pad_set_caps((struct mpipe_pad *)pad_obj, NULL);
	}
}

void mpipe_element_add_pad(struct mpipe_element *element, struct mpipe_pad *pad)
{
	__ASSERT_NO_MSG(element != NULL);
	__ASSERT_NO_MSG(pad != NULL);

	/* Set element that contains this pad */
	pad->object.container = &element->object;

	if (pad->direction == MPIPE_PAD_SRC) {
		sys_dlist_append(&element->src_pads, &pad->object.node);
	}

	if (pad->direction == MPIPE_PAD_SINK) {
		sys_dlist_append(&element->sink_pads, &pad->object.node);
	}
}

static struct mpipe_pad *mpipe_element_get_unlinked_pad(struct mpipe_element *element,
							uint8_t pad_id,
							enum mpipe_pad_direction direction)
{
	struct mpipe_object *obj;
	struct mpipe_pad *pad;

	sys_dlist_t *pads = direction == MPIPE_PAD_SRC ? &element->src_pads : &element->sink_pads;

	SYS_DLIST_FOR_EACH_CONTAINER(pads, obj, node) {
		pad = (struct mpipe_pad *)obj;
		if (pad->peer == NULL && (pad_id == UINT8_MAX || pad_id == obj->id)) {
			return pad;
		}
	}

	return NULL;
}

static int mpipe_element_link_pads(struct mpipe_element *src, uint8_t src_pad_id,
				   struct mpipe_element *sink, uint8_t sink_pad_id)
{
	struct mpipe_pad *src_pad = mpipe_element_get_unlinked_pad(src, src_pad_id, MPIPE_PAD_SRC);
	struct mpipe_pad *sink_pad =
		mpipe_element_get_unlinked_pad(sink, sink_pad_id, MPIPE_PAD_SINK);
	struct mpipe_structure scratch;

	if (src_pad == NULL || sink_pad == NULL) {
		LOG_ERR("Link failed: no free %s pad on element %u",
			src_pad == NULL ? "src" : "sink",
			src_pad == NULL ? src->object.id : sink->object.id);
		return -EINVAL;
	}

	/*
	 * Refuse to link pads whose capabilities cannot intersect. The full
	 * intersection costs more than a dedicated check, but it only runs at
	 * link time.
	 */
	if (mpipe_structure_intersect(&src_pad->caps, &sink_pad->caps, &scratch) != 0) {
		return -ENOTSUP;
	}

	return mpipe_pad_link(src_pad, sink_pad);
}

int mpipe_element_link(struct mpipe_element *element, struct mpipe_element *next_element, ...)
{
	int ret;
	va_list args;

	va_start(args, next_element);
	while (next_element != NULL) {
		/* Connect the 1st unlinked pad of each element together */
		ret = mpipe_element_link_pads(element, UINT8_MAX, next_element, UINT8_MAX);
		if (ret != 0) {
			va_end(args);
			return ret;
		}

		element = next_element;
		next_element = va_arg(args, struct mpipe_element *);
	}
	va_end(args);

	return 0;
}

enum mpipe_state_change_return mpipe_element_set_state(struct mpipe_element *element,
						       enum mpipe_state state)
{
	__ASSERT_NO_MSG(element != NULL);

	if (element->set_state != NULL) {
		return element->set_state(element, state);
	}

	return MPIPE_STATE_CHANGE_FAILURE;
}

static enum mpipe_state_change_return mpipe_element_set_state_func(struct mpipe_element *element,
								   enum mpipe_state state)
{
	enum mpipe_state_change_return ret = MPIPE_STATE_CHANGE_SUCCESS;
	enum mpipe_state_change transition;
	enum mpipe_state next;
	enum mpipe_state *current = &element->current_state;

	while (*current != state) {
		next = MPIPE_STATE_GET_NEXT(*current, state);
		transition = MPIPE_STATE_TRANSITION(*current, next);
		ret = element->change_state(element, transition);
		/* Do not handle ASYNC yet */
		if (ret != MPIPE_STATE_CHANGE_SUCCESS) {
			return ret;
		}

		*current = next;
	}

	return ret;
}

static enum mpipe_state_change_return
mpipe_element_change_state_func(struct mpipe_element *element, enum mpipe_state_change transition)
{
	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		LOG_DBG("State changed READY -> PAUSED");
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING:
		LOG_DBG("State changed PAUSED -> PLAYING");
		break;
	case MPIPE_STATE_CHANGE_PLAYING_TO_PAUSED:
		LOG_DBG("State changed PLAYING -> PAUSED");
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		LOG_DBG("State changed PAUSED -> READY");
		break;
	default:
		LOG_ERR("State changed UNKNOWN");
		break;
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

struct zbus_channel *mpipe_element_get_bus_chan(struct mpipe_element *element)
{
	struct mpipe_object *bin;

	__ASSERT_NO_MSG(element != NULL);

	/*
	 * Only a bin owns a bus, so a bin answers with its own and anything
	 * else with the bin holding it - the nearest one in both cases. A
	 * container is only ever set by mpipe_bin_add(), so it is always a bin.
	 */
	bin = &element->object;
	if ((bin->flags & MPIPE_OBJECT_FLAG_BIN) == 0) {
		bin = bin->container;
	}

	if (bin == NULL) {
		return NULL;
	}

	return &((struct mpipe_bin *)bin)->bus.channel;
}

int mpipe_message_post(struct mpipe_message *message)
{
	struct zbus_channel *chan;

	__ASSERT_NO_MSG(message != NULL);

	if (message->origin == NULL || message->type == 0) {
		return -EINVAL;
	}

	chan = mpipe_element_get_bus_chan(message->origin);
	if (chan == NULL) {
		return -ENODEV;
	}

	return zbus_chan_pub(chan, message, K_NO_WAIT);
}

int mpipe_element_init(struct mpipe_element *self, uint8_t id)
{
	__ASSERT_NO_MSG(self != NULL);

	mpipe_object_init(&self->object);
	self->object.id = id;

	IF_ENABLED(CONFIG_MPIPE_DUMP, (self->name = NULL;))

	sys_dlist_init(&self->src_pads);
	sys_dlist_init(&self->sink_pads);

	self->current_state = MPIPE_STATE_READY;
	self->set_state = mpipe_element_set_state_func;
	self->change_state = mpipe_element_change_state_func;

	return 0;
}
