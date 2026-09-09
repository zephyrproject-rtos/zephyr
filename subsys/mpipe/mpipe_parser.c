/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_parser.h>
#include <zephyr/mpipe/mpipe_structure.h>

LOG_MODULE_REGISTER(mpipe_parser, CONFIG_MPIPE_LOG_LEVEL);

#define MPIPE_PAD_SINK_ID 0
#define MPIPE_PAD_SRC_ID  1

static int mpipe_parser_set_caps(struct mpipe_parser *parser, enum mpipe_pad_direction direction,
				 const struct mpipe_structure *caps)
{
	if (caps == NULL) {
		return -EINVAL;
	}

	if (direction == MPIPE_PAD_SINK) {
		return mpipe_pad_set_caps(&parser->sink_pad, caps);
	}

	if (direction == MPIPE_PAD_SRC) {
		return mpipe_pad_set_caps(&parser->src_pad, caps);
	}

	return -EINVAL;
}

static inline int mpipe_parser_query_caps(struct mpipe_parser *self,
					  enum mpipe_pad_direction direction,
					  struct mpipe_dispatch *query)
{
	int ret;
	struct mpipe_pad *this_pad, *other_pad;
	struct mpipe_structure other_caps;
	struct mpipe_structure candidate;

	switch (direction) {
	case MPIPE_PAD_SINK:
		this_pad = &self->sink_pad;
		other_pad = &self->src_pad;
		break;
	case MPIPE_PAD_SRC:
		this_pad = &self->src_pad;
		other_pad = &self->sink_pad;
		break;
	default:
		return -EINVAL;
	}

	/*
	 * Keep the first supported capability the query accepts. The peer is
	 * asked with the caps supported on the other side, which do not depend
	 * on the candidate, so there is nothing here to backtrack over.
	 */
	ret = mpipe_pad_enum_first(this_pad, query->caps, &candidate);
	if (ret != 0) {
		return ret;
	}

	/* Query the peer using what the other side supports */
	ret = mpipe_pad_enum_caps(other_pad, 0, NULL, &other_caps);
	if (ret != 0) {
		return ret;
	}

	*query->caps = other_caps;

	ret = mpipe_pad_query(other_pad->peer, query);
	if (ret < 0) {
		return ret;
	}

	/* Keep the query result at other_pad to use later at caps event */
	ret = mpipe_pad_set_caps(other_pad, query->caps);
	if (ret != 0) {
		return ret;
	}

	/* Answer the query, writing into the asker's storage */
	*query->caps = candidate;

	return 0;
}

static int mpipe_parser_event(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	struct mpipe_parser *parser = (struct mpipe_parser *)pad->object.container;
	struct mpipe_pad *other_pad =
		(pad->direction == MPIPE_PAD_SINK) ? &parser->src_pad : &parser->sink_pad;
	int ret;

	switch (event->type) {
	case MPIPE_DISPATCH_EOS:
		return mpipe_pad_send_event_default(pad, event);
	case MPIPE_DISPATCH_CAPS:
		/*
		 * An event carrying no capability means the upstream could not
		 * fixate. The parser knows the stream format it negotiated, so
		 * it promotes the event into a real one of its own, referencing
		 * this frame's storage.
		 */
		if (event->caps == NULL) {
			struct mpipe_structure fwd_caps = other_pad->caps;
			struct mpipe_dispatch fwd_event = {
				.type = MPIPE_DISPATCH_CAPS,
				.caps = &fwd_caps,
			};

			return mpipe_pad_send_event_default(pad, &fwd_event);
		}

		ret = mpipe_pad_set_caps(pad, event->caps);
		if (ret != 0) {
			return ret;
		}

		*event->caps = other_pad->caps;

		return mpipe_pad_send_event_default(pad, event);
	default:
		return -ENOTSUP;
	}
}

/* TODO: Make a helper to refactor this together with mpipe_transform */
static int mpipe_parser_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	if (pad == NULL || query == NULL) {
		return -EINVAL;
	}

	int ret;
	struct mpipe_parser *parser = (struct mpipe_parser *)pad->object.container;

	switch (query->type) {
	case MPIPE_DISPATCH_CAPS:
		return mpipe_parser_query_caps(parser, pad->direction, query);
	case MPIPE_DISPATCH_BUFFER_POOL:
		struct mpipe_dispatch peer_query = {
			.type = MPIPE_DISPATCH_BUFFER_POOL,
			.caps = &parser->src_pad.caps,
		};

		/* Query the downstream */
		ret = mpipe_pad_query(parser->src_pad.peer, &peer_query);
		if (ret < 0) {
			return ret;
		}

		if (parser->decide_buffer_pool != NULL) {
			ret = parser->decide_buffer_pool(parser, &peer_query);
			if (ret < 0) {
				return ret;
			}
		}

		/* Configure/start the output buffer pool */
		if (parser->out_pool != NULL && !parser->out_pool->started) {
			ret = mpipe_buffer_pool_configure(parser->out_pool, &parser->src_pad.caps);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to configure output parser buffer pool");
				return ret;
			}

			ret = mpipe_buffer_pool_start(parser->out_pool);
			if (ret != 0 && ret != -ENOSYS) {
				LOG_ERR("Failed to start output parser buffer pool");
				return ret;
			}
		}

		/* Propose the buffer pool to upstream */
		if (parser->propose_buffer_pool != NULL) {
			return parser->propose_buffer_pool(parser, query);
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

enum mpipe_state_change_return mpipe_parser_change_state(struct mpipe_element *self,
							 enum mpipe_state_change transition)
{
	struct mpipe_parser *parser = (struct mpipe_parser *)self;

	switch (transition) {
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		mpipe_element_reset_pad_caps(self);

		/*
		 * The parser started the pool it draws from - its own or an
		 * adopted proposal - so it stops it here; otherwise the pool
		 * stays started across runs and can never be reconfigured.
		 */
		if (parser->out_pool != NULL) {
			(void)mpipe_buffer_pool_stop(parser->out_pool);
		}
		break;
	default:
		break;
	}

	return MPIPE_STATE_CHANGE_SUCCESS;
}

int mpipe_parser_init(struct mpipe_parser *parser, uint8_t id)
{
	__ASSERT_NO_MSG(parser != NULL);

	struct mpipe_element *self = &parser->element;
	int ret = mpipe_element_init(self, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "parser");

	mpipe_pad_init(&parser->sink_pad, MPIPE_PAD_SINK_ID, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &parser->sink_pad);

	mpipe_pad_init(&parser->src_pad, MPIPE_PAD_SRC_ID, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &parser->src_pad);

	parser->out_pool = NULL;
	self->change_state = mpipe_parser_change_state;
	parser->set_caps = mpipe_parser_set_caps;
	parser->src_pad.query_fn = mpipe_parser_query;
	parser->sink_pad.query_fn = mpipe_parser_query;
	parser->src_pad.event_fn = mpipe_parser_event;
	parser->sink_pad.event_fn = mpipe_parser_event;
	parser->decide_buffer_pool = NULL;
	parser->propose_buffer_pool = NULL;

	return 0;
}
