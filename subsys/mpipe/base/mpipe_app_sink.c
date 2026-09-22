/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/base/mpipe_app_sink.h>

LOG_MODULE_REGISTER(mpipe_app_sink, CONFIG_MPIPE_LOG_LEVEL);

/* Deliver one buffer: to the callback, or to the pull queue */
static void mpipe_app_sink_deliver(struct mpipe_app_sink *app_sink, struct net_buf *buf)
{
	if (app_sink->cb.fn != NULL) {
		app_sink->cb.fn(buf, app_sink->cb.user_data);
		net_buf_unref(buf);
		return;
	}

	/* No callback: queue for pulling, dropping when the application lags */
	if (k_msgq_put(&app_sink->msgq, &buf, K_NO_WAIT) != 0) {
		LOG_DBG("Pull queue full, dropping buffer");
		net_buf_unref(buf);
	}
}

static int mpipe_app_sink_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				   struct net_buf **out_buf)
{
	struct mpipe_app_sink *app_sink = (struct mpipe_app_sink *)pad->object.container;
	struct net_buf *cur = in_buf;
	struct net_buf *next;

	*out_buf = NULL;

	/* A chain carries independent buffers of the same caps: deliver each on its own */
	while (cur != NULL) {
		next = cur->frags;
		cur->frags = NULL;
		mpipe_app_sink_deliver(app_sink, cur);
		cur = next;
	}

	return 0;
}

int mpipe_app_sink_pull(struct mpipe_app_sink *app_sink, struct net_buf **buf, k_timeout_t timeout)
{
	if (app_sink == NULL || buf == NULL) {
		return -EINVAL;
	}

	if (k_msgq_get(&app_sink->msgq, buf, timeout) != 0) {
		return -EAGAIN;
	}

	return 0;
}

/* The one capability the element accepts is the one the application declared */
static int mpipe_app_sink_enum_caps(struct mpipe_pad *pad, uint32_t index,
				    const struct mpipe_structure *filter,
				    struct mpipe_structure *out)
{
	struct mpipe_app_sink *app_sink = (struct mpipe_app_sink *)pad->object.container;

	if (index > 0U) {
		return -ENOENT;
	}

	return mpipe_pad_enum_filter(&app_sink->caps, filter, out);
}

static int mpipe_app_sink_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_app_sink *app_sink = (struct mpipe_app_sink *)obj;

	if (val == NULL) {
		return -EINVAL;
	}

	switch (key) {
	case MPIPE_PROP_BASE_APP_SINK_CAPS:
		app_sink->caps = *(const struct mpipe_structure *)val;
		return 0;
	case MPIPE_PROP_BASE_APP_SINK_CB:
		app_sink->cb = *(const struct mpipe_app_sink_cb *)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

static int mpipe_app_sink_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_app_sink *app_sink = (struct mpipe_app_sink *)obj;

	if (val == NULL) {
		return -EINVAL;
	}

	switch (key) {
	case MPIPE_PROP_BASE_APP_SINK_CAPS:
		*(const struct mpipe_structure **)val = &app_sink->caps;
		return 0;
	case MPIPE_PROP_BASE_APP_SINK_CB:
		*(struct mpipe_app_sink_cb *)val = app_sink->cb;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

static int mpipe_app_sink_change_state(struct mpipe_element *element,
				       enum mpipe_state_change transition)
{
	struct mpipe_app_sink *app_sink = (struct mpipe_app_sink *)element;
	struct net_buf *buf;

	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		/* Release anything the application never pulled */
		while (k_msgq_get(&app_sink->msgq, &buf, K_NO_WAIT) == 0) {
			net_buf_unref(buf);
		}
		break;
	default:
		break;
	}

	return mpipe_sink_change_state(element, transition);
}

int mpipe_app_sink_init(struct mpipe_app_sink *app_sink, uint8_t id)
{
	__ASSERT_NO_MSG(app_sink != NULL);

	struct mpipe_element *self = &app_sink->sink.element;
	int ret = mpipe_sink_init(&app_sink->sink, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "app_sink");

	self->object.set_property = mpipe_app_sink_set_property;
	self->object.get_property = mpipe_app_sink_get_property;
	self->change_state = mpipe_app_sink_change_state;

	ret = mpipe_structure_init_any(&app_sink->caps);
	if (ret != 0) {
		return ret;
	}

	app_sink->sink.sink_pad.enum_caps_fn = mpipe_app_sink_enum_caps;
	app_sink->sink.sink_pad.chain_fn = mpipe_app_sink_chain_fn;

	app_sink->cb.fn = NULL;
	app_sink->cb.user_data = NULL;

	k_msgq_init(&app_sink->msgq, app_sink->msgq_buffer, sizeof(void *),
		    CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH);

	return 0;
}
