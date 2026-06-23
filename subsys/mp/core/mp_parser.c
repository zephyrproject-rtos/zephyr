/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_parser.h>

LOG_MODULE_REGISTER(mp_parser, CONFIG_MP_LOG_LEVEL);

#define MP_PAD_SINK_ID 0
#define MP_PAD_SRC_ID  1

void mp_parser_update_caps(struct mp_parser *parser, struct mp_caps *sink_caps,
			   struct mp_caps *src_caps)
{
	mp_caps_replace(&parser->sink_caps, sink_caps);
	mp_caps_replace(&parser->sinkpad.caps, parser->sink_caps);
	mp_caps_replace(&parser->src_caps, src_caps);
	mp_caps_replace(&parser->srcpad.caps, parser->src_caps);
}

static struct mp_caps *mp_parser_get_caps(struct mp_parser *parser, enum mp_pad_direction direction)
{
	if (direction == MP_PAD_SINK) {
		return mp_caps_ref(parser->sink_caps);
	}

	if (direction == MP_PAD_SRC) {
		return mp_caps_ref(parser->src_caps);
	}

	return NULL;
}

static int mp_parser_set_caps(struct mp_parser *parser, enum mp_pad_direction direction,
			      struct mp_caps *caps)
{
	if (caps == NULL) {
		return -EINVAL;
	}

	if (direction == MP_PAD_SINK) {
		mp_caps_replace(&parser->sinkpad.caps, caps);
		return 0;
	}

	if (direction == MP_PAD_SRC) {
		mp_caps_replace(&parser->srcpad.caps, caps);
		return 0;
	}

	return -EINVAL;
}

static inline int mp_parser_query_caps(struct mp_parser *self, enum mp_pad_direction direction,
				       struct mp_dispatch *query)
{
	int ret;
	struct mp_pad *other_pad;
	struct mp_caps *queried_pad_caps;
	struct mp_caps *this_caps = (direction == MP_PAD_SINK) ? self->sink_caps : self->src_caps;
	struct mp_caps *other_caps = (direction == MP_PAD_SINK) ? self->src_caps : self->sink_caps;

	switch (direction) {
	case MP_PAD_SINK:
		other_pad = &self->srcpad;
		break;
	case MP_PAD_SRC:
		other_pad = &self->sinkpad;
		break;
	default:
		return -EINVAL;
	}

	struct mp_caps *query_caps = mp_dispatch_get_caps(query);

	queried_pad_caps = mp_caps_intersect(query_caps, this_caps);
	mp_caps_unref(query_caps);
	if (queried_pad_caps == NULL) {
		return -ENODATA;
	}
	if (mp_caps_is_empty(queried_pad_caps)) {
		mp_caps_unref(queried_pad_caps);
		return -ENODATA;
	}

	/* Query the peer using the other side supported caps */
	ret = mp_dispatch_set_caps(query, other_caps);
	if (ret < 0) {
		mp_caps_unref(queried_pad_caps);
		return ret;
	}

	ret = mp_pad_query(other_pad->peer, query);
	if (ret < 0) {
		mp_caps_unref(queried_pad_caps);
		return ret;
	}

	query_caps = mp_dispatch_get_caps(query);
	/* Keep query_caps result at other_pad to use later at caps event */
	mp_caps_replace(&other_pad->caps, query_caps);
	mp_caps_unref(query_caps);

	/* Answer the query */
	ret = mp_dispatch_set_caps(query, queried_pad_caps);
	mp_caps_unref(queried_pad_caps);

	return ret;
}

static int mp_parser_event(struct mp_pad *pad, struct mp_dispatch *event)
{
	struct mp_parser *parser = (struct mp_parser *)pad->object.container;
	struct mp_pad *other_pad =
		(pad->direction == MP_PAD_SINK) ? &parser->srcpad : &parser->sinkpad;
	int ret;

	switch (event->type) {
	case MP_DISPATCH_EOS:
		return mp_pad_send_event_default(pad, event);
	case MP_DISPATCH_CAPS:
		struct mp_caps *evt_caps = mp_dispatch_get_caps(event);

		mp_caps_replace(&pad->caps, evt_caps);
		ret = mp_dispatch_set_caps(event, other_pad->caps);
		if (ret < 0) {
			return ret;
		}

		mp_caps_unref(evt_caps);

		return mp_pad_send_event_default(pad, event);
	default:
		return -ENOTSUP;
	}
}

/* TODO: Make a helper to refactor this together with mp_transform */
static int mp_parser_query(struct mp_pad *pad, struct mp_dispatch *query)
{
	if (pad == NULL || query == NULL) {
		return -EINVAL;
	}

	int ret;
	struct mp_parser *parser = (struct mp_parser *)pad->object.container;

	switch (query->type) {
	case MP_DISPATCH_CAPS:
		return mp_parser_query_caps(parser, pad->direction, query);
	case MP_DISPATCH_BUFFER_CONFIG:
		struct mp_dispatch peer_query;

		mp_dispatch_buffer_config_init(&peer_query, parser->srcpad.caps);

		/* Query the downstream */
		ret = mp_pad_query(parser->srcpad.peer, &peer_query);
		if (ret < 0) {
			mp_dispatch_clear(&peer_query);
			return ret;
		}

		if (parser->decide_allocation != NULL) {
			ret = parser->decide_allocation(parser, &peer_query);
			if (ret < 0) {
				mp_dispatch_clear(&peer_query);
				return ret;
			}

			mp_dispatch_clear(&peer_query);
		}

		/* Configure/start the output buffer pool */
		if (parser->outpool != NULL && !parser->outpool->started) {
			ret = mp_buffer_pool_configure(
				parser->outpool, mp_caps_get_structure(parser->srcpad.caps, 0));
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to configure output parser buffer pool");
				return ret;
			}

			ret = mp_buffer_pool_start(parser->outpool);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to start output parser buffer pool");
				return ret;
			}
		}

		/* Propose allocation to upstream */
		if (parser->propose_allocation != NULL) {
			return parser->propose_allocation(parser, query);
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

void mp_parser_init(struct mp_element *self)
{
	struct mp_parser *parser = (struct mp_parser *)self;

	/* Default supported caps */
	parser->sink_caps = mp_caps_new_any();
	parser->src_caps = mp_caps_new_any();

	mp_pad_init(&parser->sinkpad, MP_PAD_SINK_ID, MP_PAD_SINK, MP_PAD_ALWAYS,
		    parser->sink_caps);
	mp_element_add_pad(self, &parser->sinkpad);

	mp_pad_init(&parser->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS, parser->src_caps);
	mp_element_add_pad(self, &parser->srcpad);

	parser->outpool = NULL;
	parser->get_caps = mp_parser_get_caps;
	parser->set_caps = mp_parser_set_caps;
	parser->srcpad.queryfn = mp_parser_query;
	parser->sinkpad.queryfn = mp_parser_query;
	parser->srcpad.eventfn = mp_parser_event;
	parser->sinkpad.eventfn = mp_parser_event;
	parser->decide_allocation = NULL;
	parser->propose_allocation = NULL;
}
