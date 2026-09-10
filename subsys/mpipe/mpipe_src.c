/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_structure.h>

LOG_MODULE_REGISTER(mpipe_src, CONFIG_MPIPE_LOG_LEVEL);

#define MPIPE_PAD_SRC_ID 0

int mpipe_src_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_src *src = (struct mpipe_src *)obj;

	switch (key) {
	case MPIPE_PROP_SRC_NUM_BUFS:
		src->num_buffers = (uint32_t)(uintptr_t)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

int mpipe_src_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_src *src = (struct mpipe_src *)obj;

	switch (key) {
	case MPIPE_PROP_SRC_NUM_BUFS:
		*(uint32_t *)val = src->num_buffers;
		break;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}

	return 0;
}

static int mpipe_src_set_caps(struct mpipe_src *src, const struct mpipe_structure *caps)
{
	if (src == NULL) {
		return -EINVAL;
	}

	return mpipe_pad_set_caps(&src->src_pad, caps);
}

static int mpipe_src_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	switch (query->type) {
	case MPIPE_DISPATCH_CAPS:
		return mpipe_pad_answer_caps_query(pad, query);
	default:
		return -ENOTSUP;
	}
}

/*
 * Offer one candidate to the downstream peer and, when the peer accepts it,
 * keep the result on the src pad. Reports -ENODATA when the peer has nothing
 * in common with this candidate, which is the caller's cue to offer the next.
 */
static int mpipe_src_offer_candidate(struct mpipe_src *src, const struct mpipe_structure *candidate)
{
	/*
	 * The query references this frame's storage for its whole walk; the
	 * peer answers by writing into it, so the caller's candidate is copied
	 * rather than referenced.
	 */
	struct mpipe_structure caps_storage = *candidate;
	struct mpipe_dispatch caps_query = {
		.type = MPIPE_DISPATCH_CAPS,
		.caps = &caps_storage,
	};
	int ret;

	ret = mpipe_pad_query(src->src_pad.peer, &caps_query);
	if (ret != 0) {
		return -ENODATA;
	}

	if (mpipe_structure_is_empty(&caps_storage)) {
		return -ENODATA;
	}

	/* Store negotiated (possibly unfixed) caps on the src pad */
	return mpipe_pad_set_caps(&src->src_pad, &caps_storage);
}

static int mpipe_src_negotiate(struct mpipe_src *src)
{
	struct mpipe_structure candidate;
	struct mpipe_structure fixated;
	struct mpipe_structure event_caps;
	uint32_t index;
	bool is_fixated;
	int ret;

	/*
	 * Offer the supported capabilities one at a time and keep the first one
	 * the peer accepts. Offering them individually is what allows a rejected
	 * candidate to be retried with the next one instead of the whole
	 * negotiation failing, and it keeps the peer's answer down to what one
	 * candidate can match.
	 */
	for (index = 0;; index++) {
		ret = mpipe_pad_enum_caps(&src->src_pad, index, NULL, &candidate);
		if (ret == -EAGAIN) {
			continue;
		}

		if (ret == -ENOENT) {
			/*
			 * An element with no capability at all cannot negotiate,
			 * which is a caller error rather than a failed negotiation.
			 */
			return (index == 0) ? -EINVAL : -ENODATA;
		}

		if (ret != 0) {
			return ret;
		}

		ret = mpipe_src_offer_candidate(src, &candidate);
		if (ret == 0) {
			break;
		}

		if (ret != -ENODATA) {
			return ret;
		}
	}

	is_fixated = (mpipe_structure_fixate(&src->src_pad.caps, &fixated) == 0);

	/*
	 * Push a caps event downstream. The result only matters when a fixated
	 * capability was sent; an event carrying no capability is informational.
	 * The event references a sacrificial copy: elements replace the event's
	 * capability with the one that crosses them, and the source still needs
	 * the fixated one afterwards.
	 */
	if (is_fixated) {
		event_caps = fixated;
	}

	struct mpipe_dispatch caps_event = {
		.type = MPIPE_DISPATCH_CAPS,
		.caps = is_fixated ? &event_caps : NULL,
	};

	ret = mpipe_pad_send_event(src->src_pad.peer, &caps_event);

	/* Apply the fixated capability to the element itself */
	if (is_fixated) {
		if (ret == 0) {
			ret = src->set_caps(src, &fixated);
		}

		if (ret != 0) {
			return ret;
		}
	}

	/* Query the peer's buffer pool proposal */
	struct mpipe_dispatch pool_query = {
		.type = MPIPE_DISPATCH_BUFFER_POOL,
		.caps = &src->src_pad.caps,
	};

	ret = mpipe_pad_query(src->src_pad.peer, &pool_query);
	if (ret != 0) {
		return ret;
	}

	/* Decide the buffer pool */
	if (src->decide_buffer_pool != NULL) {
		return src->decide_buffer_pool(src, &pool_query);
	}

	return 0;
}

enum mpipe_state_change_return mpipe_src_change_state(struct mpipe_element *self,
						      enum mpipe_state_change transition)
{
	struct mpipe_src *src = (struct mpipe_src *)self;
	enum mpipe_state_change_return ret = MPIPE_STATE_CHANGE_SUCCESS;
	int neg_ret;
	int pool_ret;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		/* Perform negotiation */
		neg_ret = mpipe_src_negotiate(src);
		if (neg_ret < 0) {
			struct mpipe_message msg = {
				.origin = self,
				.type = MPIPE_MESSAGE_ERROR,
				.domain = MPIPE_ERROR_CAPS,
				.code = neg_ret,
			};

			LOG_ERR("Negotiation failed");
			(void)mpipe_message_post(&msg);
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		/* Config buffer pool */
		pool_ret = mpipe_buffer_pool_configure(src->pool, &src->src_pad.caps);
		if (pool_ret != 0 && pool_ret != -ENOSYS) {
			struct mpipe_message msg = {
				.origin = self,
				.type = MPIPE_MESSAGE_ERROR,
				.domain = MPIPE_ERROR_BUFFER_POOL,
				.code = pool_ret,
			};

			LOG_ERR("Failed to configure source buffer pool");
			(void)mpipe_message_post(&msg);
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		/* Start buffer pool */
		pool_ret = mpipe_buffer_pool_start(src->pool);
		if (pool_ret != 0 && pool_ret != -ENOSYS) {
			struct mpipe_message msg = {
				.origin = self,
				.type = MPIPE_MESSAGE_ERROR,
				.domain = MPIPE_ERROR_BUFFER_POOL,
				.code = pool_ret,
			};

			LOG_ERR("Failed to start source buffer pool");
			(void)mpipe_message_post(&msg);
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		/*
		 * Stop the buffer pool on teardown. This is the counterpart of
		 * the READY_TO_PAUSED start above and is what makes stop/replay
		 * symmetric: e.g. the video pool issues video_stream_stop() and
		 * releases its driver buffers here so a subsequent replay can
		 * start streaming cleanly. A pool without a stop hook returns
		 * -ENOSYS, which is not an error.
		 */
		pool_ret = mpipe_buffer_pool_stop(src->pool);
		if (pool_ret != 0 && pool_ret != -ENOSYS) {
			LOG_ERR("Failed to stop source buffer pool");
			return MPIPE_STATE_CHANGE_FAILURE;
		}

		mpipe_element_reset_pad_caps(self);

		break;
	default:
		break;
	}

	return ret;
}

int mpipe_src_init(struct mpipe_src *src, uint8_t id)
{
	__ASSERT_NO_MSG(src != NULL);

	struct mpipe_element *self = &src->element;
	int ret = mpipe_element_init(self, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "src");

	mpipe_pad_init(&src->src_pad, MPIPE_PAD_SRC_ID, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(self, &src->src_pad);

	self->object.set_property = mpipe_src_set_property;
	self->object.get_property = mpipe_src_get_property;
	self->change_state = mpipe_src_change_state;

	src->set_caps = mpipe_src_set_caps;
	src->src_pad.query_fn = mpipe_src_query;
	src->decide_buffer_pool = NULL;

	return 0;
}
